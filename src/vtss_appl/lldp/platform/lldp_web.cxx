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

#include "mgmt_api.h"
#include "web_api.h"
#include "lldp_api.h"
#include "lldp_remote.h"
#include "lldp_tlv.h"
#ifdef VTSS_SW_OPTION_LLDP_MED
#include "lldpmed_rx.h"
#include "lldpmed_tx.h"
#endif
#ifdef VTSS_SW_OPTION_LLDP_ORG
#include "lldporg_spec_tlvs_rx.h"
#endif
#include "l2proto_api.h"
#ifdef VTSS_SW_OPTION_AGGR
#include "aggr_api.h"
#endif
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_web_api.h"
#endif
#include "vlan_api.h" // For VTSS_APPL_VLAN_ID_Mxx

#ifdef VTSS_SW_OPTION_EEE
#include "eee_api.h"
#endif
#define LLDP_WEB_BUF_LEN 512

/*lint -esym(459, lldp_msg_id_get_all_entries_req_flags*/ // Protect by vtss_appl_lldp_mutex_lock
/*lint -esym(459,var_clear)*/
/*lint -esym(459,err_msg)*/

/* =================
* Trace definitions
* -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB
#include <vtss_trace_api.h>
/* ============== */

//#define DO_WEB_TEST(p, module)  if (do_web_test(p, module) == -1) { return -1; }

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/

// Function that returns the value from a web form containing a long. It checks
// if the value is within an allowed range given by min_value and max_value (both
// values included) . If the value isn't within the allowed ranged an error message
// is thrown, and the minimum value is returned.
#ifdef VTSS_SW_OPTION_LLDP_MED
static ulong httpd_form_get_value_ulong_int(CYG_HTTPD_STATE *p, const char form_name[255], ulong min_value, ulong max_value)
{
    ulong form_value = 0;
    if (cyg_httpd_form_varable_long_int(p, form_name, &form_value)) {
        if (form_value < min_value || form_value > max_value) {
            T_E("Invalid value. Form name = %s, form value = " VPRIlu, form_name, form_value );
            form_value =  min_value;
        }
    } else {
        T_E("Unknown form. Form name = %s", form_name);
        form_value =  min_value;
    }

    return form_value;
}
#endif
//
// LLDP neighbors handler
//
static i32 handler_config_lldp_neighbor(CYG_HTTPD_STATE *p)
{
    vtss_isid_t         sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int                 ct;
    u32                 entry_index;
    char                no_entries_found = 1;
    vtss_appl_lldp_remote_entry_t *entry;
    vtss_appl_lldp_remote_entry_t *table = NULL;
    port_iter_t         pit;
    u8                  mgmt_addr_index;

    vtss_appl_lldp_cap_t cap;
    if (vtss_appl_lldp_cap_get(&cap) != VTSS_RC_OK) {
        T_E("Could not get capabilities");
    }
    if (sid != VTSS_ISID_START) {
        return -1;
    }

    if (web_get_method(p, VTSS_MODULE_ID_LLDP) == CYG_HTTPD_METHOD_GET) {
        cyg_httpd_start_chunked("html");

        vtss_appl_lldp_mutex_lock();

        table = vtss_appl_lldp_entries_get(); // Get the entries table.


        // Loop through all front ports
        if (port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT) == VTSS_RC_OK) {
            while (port_iter_getnext(&pit)) {

                // Sort the entries by local port.
                entry = table;       // Restart the entries
                for (entry_index = 0 ; entry_index < cap.remote_entries_cnt; entry_index++) {
                    if (entry->in_use == 0 || entry->receive_port != pit.iport) {
                        // This is the sorting of the entries
                        entry++;
                        continue;
                    }
                    if ((entry->in_use)) {
                        char chassis_string[255]     = "";
                        char capa_string[255]        = "";
                        char port_id_string[255]     = "";
                        char mgmt_addr_string[255]   = "";
                        char system_name_string[255] = "";
                        char port_descr_string[255]  = "";
                        char receive_port[255]       = "";
                        no_entries_found = 0;
                        (void) vtss_appl_lldp_chassis_id2string(entry, &chassis_string[0]);
                        lldp_remote_system_capa_to_string(entry, &capa_string[0]);
                        (void) vtss_appl_lldp_port_id2string(entry, &port_id_string[0]);
                        (void) lldp_remote_receive_port_to_string(entry->receive_port, &receive_port[0], sid);
                        (void) vtss_appl_lldp_system_name2string(entry, &system_name_string[0]);
                        (void) vtss_appl_lldp_port_descr2string(entry, &port_descr_string[0]);
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s?%s?%s?%s?%s?%s?",
                                      receive_port,
                                      chassis_string,
                                      port_id_string,
                                      system_name_string,
                                      port_descr_string,
                                      capa_string
                                     );
                        cyg_httpd_write_chunked(p->outbuffer, ct);

                        BOOL add_split = FALSE;
                        char buf[50];
                        for (mgmt_addr_index = 0; mgmt_addr_index < LLDP_MGMT_ADDR_CNT; mgmt_addr_index++) {
                            (void) vtss_lldp_mgmt_addr_and_type2string(entry, mgmt_addr_index, &mgmt_addr_string[0], 255);
                            if (entry->mgmt_addr[mgmt_addr_index].length > 0) {
                                if (add_split) {
                                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "! %s/%d/%s",
                                                  mgmt_addr_string,
                                                  entry->mgmt_addr[mgmt_addr_index].subtype,
                                                  vtss_appl_lldp_mgmt_addr2string(&entry->mgmt_addr[mgmt_addr_index], &buf[0])
                                                 );
                                } else {
                                    add_split = TRUE;
                                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s/%d/%s",
                                                  mgmt_addr_string,
                                                  entry->mgmt_addr[mgmt_addr_index].subtype,
                                                  vtss_appl_lldp_mgmt_addr2string(&entry->mgmt_addr[mgmt_addr_index], &buf[0])
                                                 );
                                }
                                cyg_httpd_write_chunked(p->outbuffer, ct);
                            }
                        }

                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|", "");
                        cyg_httpd_write_chunked(p->outbuffer, ct);

                        T_R("cyg_httpd_write_chunked -> %s", p->outbuffer);
                    }
                    entry++;
                }
            }
        }
        vtss_appl_lldp_mutex_unlock();

        if (no_entries_found) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|", "");
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

//
// LLDP-MED neighbors handler
//
#ifdef VTSS_SW_OPTION_LLDP_MED
static i32 handler_lldp_neighbor_med(CYG_HTTPD_STATE *p)
{
    vtss_isid_t         sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int                 ct;
    u32                 entry_index;
    char                no_entries_found = 1;
    vtss_appl_lldp_remote_entry_t *entry;
    vtss_appl_lldp_remote_entry_t *table = NULL;
    uint                p_index = 0;
    port_iter_t          pit;
    vtss_appl_lldp_cap_t cap;
    if (vtss_appl_lldp_cap_get(&cap) != VTSS_RC_OK) {
        T_E("Could not get capabilities");
    }
    if (sid != VTSS_ISID_START) {
        return -1;
    }

    if (web_get_method(p, VTSS_MODULE_ID_LLDP) == CYG_HTTPD_METHOD_GET) {

        cyg_httpd_start_chunked("html");

        vtss_appl_lldp_mutex_lock();

        table = vtss_appl_lldp_entries_get(); // Get the entries table.

        // Loop through all front ports
        if (port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT) == VTSS_RC_OK) {
            while (port_iter_getnext(&pit)) {

                // Sort the entries by local port.
                entry = table;       // Restart the entries
                for (entry_index = 0 ; entry_index < cap.remote_entries_cnt; entry_index++) {
                    if (entry->in_use == 0 || entry->receive_port != pit.iport) {
                        // This is the sorting of the entries
                        entry++;
                        continue;
                    }

                    if (entry->lldpmed_info_vld) {
                        no_entries_found = 0;
                        // Port + Capability
                        char device_type_str[255] = "";
                        lldpmed_device_type2str(entry, &device_type_str[0]);

                        char capa_str[255] = "";
                        lldpmed_capabilities2str(entry, &capa_str[0]);

                        char receive_port_str[255]       = "";
                        (void) lldp_remote_receive_port_to_string(entry->receive_port, &receive_port_str[0], sid);

                        // Local Port
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s?",
                                      receive_port_str);
                        cyg_httpd_write_chunked(p->outbuffer, ct);

                        T_D("device_type_str = %s", device_type_str);
                        // Device type + capabilities
                        if (entry->lldpmed_capabilities_vld) {
                            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s&%s",
                                          device_type_str,
                                          capa_str
                                         );
                            cyg_httpd_write_chunked(p->outbuffer, ct);
                        }

                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "?%d~",
                                      lldpmed_get_policies_cnt(entry));
                        cyg_httpd_write_chunked(p->outbuffer, ct);
                        if (lldpmed_get_policies_cnt(entry) > 0) {
                            for (p_index = 0; p_index < VTSS_APPL_LLDP_MED_POLICY_APPLICATIONS_CNT; p_index ++) {
                                // Policies
                                char appl_str[50];
                                char vlan_str[50];
                                char prio_str[50];
                                char dscp_str[50];

                                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s&%s&%s&%s&%s&%s~",
                                              lldpmed_appl_type2str(entry->policy[p_index].network_policy.application_type, &appl_str[0]),
                                              lldpmed_policy_flag_type2str(entry->policy[p_index].network_policy.unknown_policy_flag),
                                              lldpmed_policy_tag2str(entry->policy[p_index].network_policy.tagged_flag),
                                              lldpmed_policy_vlan_id2str(entry->policy[p_index], &vlan_str[0]),
                                              lldpmed_policy_prio2str(entry->policy[p_index], &prio_str[0]),
                                              lldpmed_policy_dscp2str(entry->policy[p_index], &dscp_str[0]));
                                cyg_httpd_write_chunked(p->outbuffer, ct);

                                // Check if the next policy contains valid information.
                                if (p_index < VTSS_APPL_LLDP_MED_POLICY_APPLICATIONS_CNT - 1) {
                                    if (!entry->policy[p_index + 1].in_use) {
                                        break;
                                    }
                                }
                            }
                        }

                        // location  information is split as this <location TLV 1>~<location TLV 2>~<location TLV 2>
                        char location_str_tmp[1000], location_str[1000]   = "";

                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "?");
                        cyg_httpd_write_chunked(p->outbuffer, ct);


                        if (entry->lldpmed_coordinate_location_vld) {
                            lldpmed_location2str(entry, &location_str[0], LLDPMED_LOCATION_COORDINATE);
                            (void) cgi_escape(&location_str[0], &location_str_tmp[0]);
                            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "~%s",
                                          location_str_tmp
                                         );
                            cyg_httpd_write_chunked(p->outbuffer, ct);
                        }

                        if (entry->lldpmed_civic_location_vld) {
                            lldpmed_location2str(entry, &location_str[0], LLDPMED_LOCATION_CIVIC);
                            (void) cgi_escape(&location_str[0], &location_str_tmp[0]);
                            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "~%s",
                                          location_str_tmp
                                         );
                            cyg_httpd_write_chunked(p->outbuffer, ct);
                        }

                        if (entry->lldpmed_elin_location_vld) {
                            lldpmed_location2str(entry, &location_str[0], LLDPMED_LOCATION_ECS);
                            (void) cgi_escape(&location_str[0], &location_str_tmp[0]);
                            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "~%s",
                                          location_str_tmp
                                         );

                            cyg_httpd_write_chunked(p->outbuffer, ct);
                        }


                        //
                        // MAC_PHY
                        //
                        cyg_httpd_write_chunked("?", 1); // Insert "splitter" for MAC_PHY data

                        if (entry->lldporg_autoneg_vld) {
                            // mac_phy conf is split as this <Autoneg Support>~<Autoneg status>~<Autoneg capa>~<Mau type>

                            char lldporg_str[255]     = "";
                            lldporg_autoneg_support2str(entry, &lldporg_str[0]);
                            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s", lldporg_str);
                            cyg_httpd_write_chunked(p->outbuffer, ct);
                            cyg_httpd_write_chunked("~", 1); // Insert "splitter" between <Autoneg Support> and <Autoneg status>

                            lldporg_autoneg_status2str(entry, &lldporg_str[0]);
                            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s", lldporg_str);
                            cyg_httpd_write_chunked(p->outbuffer, ct);
                            cyg_httpd_write_chunked("~", 1); // Insert "splitter" between <Autoneg status> and <Autoneg capa>

                            lldporg_autoneg_capa2str(entry, &lldporg_str[0]);
                            if (strlen(lldporg_str) > 0) {
                                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s", lldporg_str);
                                cyg_httpd_write_chunked(p->outbuffer, ct);
                            }
                            cyg_httpd_write_chunked("~", 1); // Insert "splitter" between <Autoneg capa> and <mau type>

                            lldporg_operational_mau_type2str(entry, &lldporg_str[0]);
                            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s", lldporg_str);
                            cyg_httpd_write_chunked(p->outbuffer, ct);
                        }


                        // Next Entry
                        cyg_httpd_write_chunked("|", 1);
                    }
                    entry++;
                }
            }
        }
        vtss_appl_lldp_mutex_unlock();

        if (no_entries_found) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|", "");
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();

    }
    return -1; // Do not further search the file system.
}

static i32 handler_lldpmed_config(CYG_HTTPD_STATE *p)
{
    vtss_isid_t           sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    uchar                 p_index = 0;
    int                   ct;
    char                  form_name[100];
    int                   form_value;
    size_t                len;
    BOOL                  insert_comma;
    static char           err_msg[100]; // Need to be static because it is set when posting data, but used getting data.
    port_iter_t           pit;
    mesa_rc               rc = VTSS_RC_OK;

    T_I("lldpmed_config  web access - SID:%d", sid);
    if (sid != VTSS_ISID_START) {
        redirect(p, "/lldp_med_config.htm");
        return -1;
    }
    const char *str;
    char tmp_str[255];
    int ca_index = 0;

    vtss_appl_lldp_common_conf_t conf;
    (void) vtss_appl_lldp_common_conf_get(&conf); // Get current configuration

    CapArray<vtss_appl_lldp_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_conf;
    (void )lldp_mgmt_conf_get(&port_conf[0]); // Get current configuration

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_LLDP)) {
        return -1;
    }
#endif

    if (web_get_method(p, VTSS_MODULE_ID_LLDP) == CYG_HTTPD_METHOD_POST) {

        strcpy(err_msg, ""); // No errors so far :)
        //
        // Setting new configuration
        //

        // Get information from WEB
        conf.medFastStartRepeatCount     = httpd_form_get_value_int(p, "fast_start_repeat_count_value",
                                                                    VTSS_APPL_LLDP_FAST_START_REPEAT_COUNT_MIN,
                                                                    VTSS_APPL_LLDP_FAST_START_REPEAT_COUNT_MAX); // Limits defined in medFastStartRepeatCount MIB in TIA1057
        i32 latitude      = httpd_form_get_value_ulong_int(p, "latitude_integer", 0, 9000000);
        BOOL latitude_dir  = (vtss_appl_lldp_med_latitude_dir_t)httpd_form_get_value_int(p, "latitude_dir", 0, 1);
        lldpmed_cal_fraction(latitude, latitude_dir, 25, &conf.coordinate_location.latitude, TUDE_DIGIT, LLDPMED_LATITUDE_BIT_MASK);

        i32 longitude     = httpd_form_get_value_ulong_int(p, "longitude_integer", 0, 180000000);
        BOOL longitude_dir = (vtss_appl_lldp_med_longitude_dir_t)httpd_form_get_value_int(p, "longitude_dir", 0, 1);
        lldpmed_cal_fraction(longitude, longitude_dir, 25, &conf.coordinate_location.longitude, TUDE_DIGIT, LLDPMED_LONGITUDE_BIT_MASK);

        i32 altitude      = httpd_form_get_value_ulong_int(p, "altitude_integer", 0, 0xFFFFFFFF);
        conf.coordinate_location.altitude_type = (vtss_appl_lldp_med_at_type_t)httpd_form_get_value_int(p, "altitude_type", 1, 2);

        i64 altitude_i64;
        lldpmed_cal_fraction(altitude, (altitude < 0 ? TRUE : FALSE), 8, &altitude_i64, ALTITUDE_DIGIT, LLDPMED_ALTITUDE_BIT_MASK);
        conf.coordinate_location.altitude = (i32) altitude_i64;

        conf.coordinate_location.datum         = (vtss_appl_lldp_med_datum_t)httpd_form_get_value_int(p, "map_datum_type", 1, 3);
        T_I("longitude:%d, altitude:%d", longitude, altitude);


        // Definition of form names, see lldp_med_config.htm
        const char *civic_form_names[] = {"state",
                                          "county",
                                          "city",
                                          "city_district",
                                          "block",
                                          "street",
                                          "leading_street_direction",
                                          "trailing_street_suffix",
                                          "str_suf",
                                          "house_no",
                                          "house_no_suffix",
                                          "landmark",
                                          "additional_info",
                                          "name",
                                          "zip_code",
                                          "building",
                                          "apartment",
                                          "floor",
                                          "room_number",
                                          "place_type",
                                          "postal_com_name",
                                          "p_o_box",
                                          "additional_code"
                                         };

        // Country code
        str = cyg_httpd_form_varable_string(p, "country_code", &len);
        strcpy(tmp_str, "");

        if (len > 0) {
            (void) cgi_unescape(str, tmp_str, len, sizeof(tmp_str));
        }

        misc_strncpyz(conf.ca_country_code, tmp_str, VTSS_APPL_LLDP_CA_COUNTRY_CODE_LEN);

        T_I("Ca value");
        // Ca value
        for (ca_index = 0; ca_index < VTSS_APPL_LLDP_MED_CATYPE_CNT; ca_index++ ) {
            str = cyg_httpd_form_varable_string(p, civic_form_names[ca_index], &len);

            strcpy(tmp_str, "");
            (void) cgi_unescape(str, tmp_str, len, sizeof(tmp_str));

            if (vtss_appl_lldp_location_civic_info_set(&conf.civic,
                                                       lldpmed_index2catype(ca_index),
                                                       tmp_str) != VTSS_RC_OK) {
                strcpy(err_msg, "Total information for Civic Address Location exceeds maximum allowed characters");
                goto stop;
            }
        }

        str = cyg_httpd_form_varable_string(p, "ecs", &len);
        memcpy(conf.elin_location, str, len);
        conf.elin_location[len] = '\0';

        //
        // Policies
        //
        T_I("Policies");
        vtss_appl_lldp_med_policy_t policy;
        for (p_index = LLDPMED_POLICY_MIN ; p_index <= LLDPMED_POLICY_MAX; p_index++ ) {
            // Get current configuration. We only change some fields.
            if ((rc = vtss_appl_lldp_conf_policy_get(p_index, &policy)) != VTSS_RC_OK) {
                T_E("Could not get policy - %s", error_txt(rc));
                goto stop;
            }

            // We only need to check that one of the forms in the row exists
            sprintf(form_name, "application_type_%u", p_index); // Set form name
            if (cyg_httpd_form_varable_int(p, form_name, &form_value)) {
                policy.network_policy.application_type = (vtss_appl_lldp_med_application_type_t)httpd_form_get_value_int(p, form_name, 1, 8);
                sprintf(form_name, "tag_%u", p_index); // Set form name
                policy.network_policy.tagged_flag = httpd_form_get_value_int(p, form_name, 0, 1);

                T_I("VLAN:%d", policy.network_policy.vlan_id);
                // VLAN ID Shall be 0 when untagged, TIA1057 table 13.
                if (policy.network_policy.tagged_flag) {
                    sprintf(form_name, "vlan_id_%u", p_index); // Set form name
                    policy.network_policy.vlan_id = httpd_form_get_value_int(p, form_name, VTSS_APPL_VLAN_ID_MIN, VTSS_APPL_VLAN_ID_MAX);

                    sprintf(form_name, "l2_priority_%u", p_index); // Set form name
                    policy.network_policy.l2_priority = httpd_form_get_value_int(p, form_name, VTSS_APPL_LLDP_MED_L2_PRIORITY_MIN, VTSS_APPL_LLDP_MED_L2_PRIORITY_MAX);
                }

                sprintf(form_name, "dscp_value_%u", p_index); // Set form name
                policy.network_policy.dscp_value = httpd_form_get_value_int(p, form_name, VTSS_APPL_LLDP_MED_DSCP_MIN, VTSS_APPL_LLDP_MED_DSCP_MAX);

                //"Delete" the policy if the delete checkbox is checked
                sprintf(form_name, "Delete_%u", p_index); // Set form name
                if (cyg_httpd_form_varable_find(p, form_name) ) {
                    policy.in_use = FALSE;
                } else {
                    policy.in_use = TRUE; // Ok - Now this policy is in use.
                }

                T_I("VLAN:%d", policy.network_policy.vlan_id);
                if ((rc = vtss_appl_lldp_conf_policy_set(p_index, policy)) != VTSS_RC_OK) {
                    T_E("Could not set policy - %s", error_txt(rc));
                    goto stop;
                }
            }
        }

        //
        // Ports
        //
        // Loop through all front ports
        T_R("sid:%d", sid);
        if (port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT) == VTSS_RC_OK) {
            while (port_iter_getnext(&pit)) {
                T_R("iport:%d", pit.iport);
                vtss_ifindex_t ifindex;
                if (vtss_ifindex_from_port(sid, pit.iport, &ifindex) != VTSS_RC_OK) {
                    T_E("Could not get ifindex");
                }

                port_conf[pit.iport].lldpmed_optional_tlvs_mask = 0;
                sprintf(form_name, "transmit_tlvs_capa_%u", pit.iport); // Set form name
                if (cyg_httpd_form_varable_find(p, form_name)) {
                    T_N_PORT(pit.iport, "form_name:%s", form_name);
                    port_conf[pit.iport].lldpmed_optional_tlvs_mask |= VTSS_APPL_LLDP_MED_OPTIONAL_TLV_CAPABILITIES_BIT;
                }

                sprintf(form_name, "transmit_tlvs_policy_%u", pit.iport); // Set form name
                if (cyg_httpd_form_varable_find(p, form_name)) {
                    T_N_PORT(pit.iport, "form_name:%s", form_name);
                    port_conf[pit.iport].lldpmed_optional_tlvs_mask |= VTSS_APPL_LLDP_MED_OPTIONAL_TLV_POLICY_BIT;
                }

                sprintf(form_name, "transmit_tlvs_location_%u", pit.iport); // Set form name
                if (cyg_httpd_form_varable_find(p, form_name)) {
                    T_N_PORT(pit.iport, "form_name:%s", form_name);
                    port_conf[pit.iport].lldpmed_optional_tlvs_mask |= VTSS_APPL_LLDP_MED_OPTIONAL_TLV_LOCATION_BIT;
                }

#ifdef VTSS_SW_OPTION_POE
                sprintf(form_name, "transmit_tlvs_poe_%u", pit.iport); // Set form name
                if (cyg_httpd_form_varable_find(p, form_name)) {
                    T_N("form_name:%s", form_name);
                    port_conf[pit.iport].lldpmed_optional_tlvs_mask |= VTSS_APPL_LLDP_MED_OPTIONAL_TLV_POE_BIT;
                }
#endif

                sprintf(form_name, "device_type_%u", pit.iport); // Set form name
                if (cyg_httpd_form_varable_int(p, form_name, &form_value) && (form_value >= 0 && form_value < 2)) {
                    // form_value ok
                    port_conf[pit.iport].lldpmed_device_type = (vtss_appl_lldp_med_device_type_t)form_value;
                } else {
                    T_E("Unknown value - form:%s, value:%d", form_name, form_value);
                    port_conf[pit.iport].lldpmed_device_type = VTSS_APPL_LLDP_MED_CONNECTIVITY;
                }

                for (p_index = LLDPMED_POLICY_MIN ; p_index <= LLDPMED_POLICY_MAX; p_index++ ) {

                    // We only need to check that one of the forms in the row exists
                    sprintf(form_name, "port_policies_%u_%u", pit.iport, p_index); // Set form name

                    // The user can have deleted a policy, so we so not apply deleted policies.
                    // This is why we need to get the policy list in order to have the policy.in_use information..
                    if ((rc = vtss_appl_lldp_conf_policy_get(p_index, &policy)) != VTSS_RC_OK) {
                        T_E("Could not get policy - %s", error_txt(rc));
                        goto stop;
                    }

                    BOOL set_policy = FALSE;
                    if (cyg_httpd_form_varable_find(p, form_name) && policy.in_use) {
                        set_policy = TRUE;
                    }
                    T_R("set_policy:%d", set_policy);

                    if (vtss_appl_lldp_conf_port_policy_set(ifindex, p_index, set_policy) != VTSS_RC_OK) {
                        T_E_PORT(pit.iport, "Could not set policy index:%d, set_policy:%d", p_index, set_policy);
                    }
                }
                T_I_PORT(pit.iport, "mask:0x%X, sid:%d", port_conf[pit.iport].lldpmed_optional_tlvs_mask, sid);
            }

            if ((rc  = lldp_mgmt_conf_set(&port_conf[0])) != VTSS_RC_OK) {
                goto stop;
            };
        }


        // Update the configuration
        rc = vtss_appl_lldp_common_conf_set(&conf);

stop:
        if (rc != VTSS_RC_OK) {
            misc_strncpyz(&err_msg[0], error_txt(rc), sizeof(err_msg));
        }

        redirect(p, "/lldp_med_config.htm");

    } else if (web_get_method(p, VTSS_MODULE_ID_LLDP) == CYG_HTTPD_METHOD_GET) {
        //
        // Getting the configuration.
        //
        T_D("Getting the configuration");
        cyg_httpd_start_chunked("html");

        // The "information" is split like this:<fast start repeat count>|<Location Data>|<Policies Data>|<Ports Data>|<error message>
        // Fast Start Repeat Count
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u|", conf.medFastStartRepeatCount);
        cyg_httpd_write_chunked(p->outbuffer, ct);


        //
        // Optional TLVs mask table
        //
        // Each port is split like this: <port number>,<Policy#1>,<Policy#2>.....
        if (port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT) == VTSS_RC_OK) {
            while (port_iter_getnext(&pit)) {
                char receive_port_str[255]       = "";
                (void) lldp_remote_receive_port_to_string(pit.iport, &receive_port_str[0], sid);
                // The & sign is used as separator between the ports.
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                              "%s,%d,%d,%d"
#ifdef VTSS_SW_OPTION_POE
                              ",%d"
#endif
                              ",%d"
                              "&",
                              receive_port_str,
                              port_conf[pit.iport].lldpmed_optional_tlvs_mask & VTSS_APPL_LLDP_MED_OPTIONAL_TLV_CAPABILITIES_BIT,
                              port_conf[pit.iport].lldpmed_optional_tlvs_mask & VTSS_APPL_LLDP_MED_OPTIONAL_TLV_POLICY_BIT,
                              port_conf[pit.iport].lldpmed_optional_tlvs_mask & VTSS_APPL_LLDP_MED_OPTIONAL_TLV_LOCATION_BIT,
#ifdef VTSS_SW_OPTION_POE
                              port_conf[pit.iport].lldpmed_optional_tlvs_mask & VTSS_APPL_LLDP_MED_OPTIONAL_TLV_POE_BIT,
#endif
                              port_conf[pit.iport].lldpmed_device_type
                             );


                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|"); // Add separator between TLVs Mask table and location Table
        cyg_httpd_write_chunked(p->outbuffer, ct);

        //
        // Location Table
        //

        // Converting the *tude values to something that we can "print"
        long altitude, longitude, latitude;
        VTSS_RC(lldp_tudes_as_long(&conf, &altitude, &longitude, &latitude));

        T_I("latitude:%ld, longitude:%ld, altitude:%ld", latitude, longitude, altitude);

        // Coordinate Location information
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,%ld,%d,%ld,%d,%ld,%d,%d#",
                      TUDE_MULTIPLIER,
                      latitude,
                      get_latitude_dir(conf.coordinate_location.latitude),
                      longitude,
                      get_longitude_dir(conf.coordinate_location.longitude),
                      altitude,
                      conf.coordinate_location.altitude_type,
                      conf.coordinate_location.datum);

        cyg_httpd_write_chunked(p->outbuffer, ct);

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s!",
                      &conf.ca_country_code[0]);

        cyg_httpd_write_chunked(p->outbuffer, ct);

        for (ca_index = 0 ; ca_index < VTSS_APPL_LLDP_MED_CATYPE_CNT; ca_index ++) {
            char buf[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];

            if (vtss_appl_lldp_location_civic_info_get(&conf.civic,
                                                       lldpmed_index2catype(ca_index),
                                                       VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX,
                                                       buf) == NULL) {
                T_E("Invalid civic type:%d - skipping", ca_index);
                continue;
            };

            if (strlen(buf)) {
                (void) cgi_escape(buf, &tmp_str[0]);
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s!",
                              &tmp_str[0]);
            } else {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "!");
            }

            cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        // ecs
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%s",
                      conf.elin_location);

        cyg_httpd_write_chunked(p->outbuffer, ct);

        //
        // Policies
        //
        cyg_httpd_write_chunked("|", 1);

        vtss_appl_lldp_med_policy_t policy;
        // The policies is split like this: <policy 1>#<policy 2>#....
        // Each policy is split like this: <policy number>,<Application Type>,<Tag>,<VLAN ID>,<L2 Priority>,<DSCP Value>
        for (p_index = LLDPMED_POLICY_MIN ; p_index <= LLDPMED_POLICY_MAX; p_index++ ) {
            VTSS_RC(vtss_appl_lldp_conf_policy_get(p_index, &policy));
            if (policy.in_use) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,%u,%u,%u,%u,%u#",
                              p_index,
                              policy.network_policy.application_type,
                              policy.network_policy.tagged_flag,
                              policy.network_policy.vlan_id,
                              policy.network_policy.l2_priority,
                              policy.network_policy.dscp_value);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }

        //
        // Ports
        //
        cyg_httpd_write_chunked("|", 1);

        // The ports is split like this: <list of policies in use>&<port 1>#<port 2>#....
        insert_comma = 0;
        for (p_index = LLDPMED_POLICY_MIN ; p_index <= LLDPMED_POLICY_MAX; p_index++ ) {
            VTSS_RC(vtss_appl_lldp_conf_policy_get(p_index, &policy));
            if (policy.in_use) {
                if (insert_comma) {
                    cyg_httpd_write_chunked(",", 1);
                }
                insert_comma = 1;

                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u",
                              p_index);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
        cyg_httpd_write_chunked("&", 1);

        // Each port is split like this: <port number>,<Policy#1>,<Policy#2>.....
        if (port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT) == VTSS_RC_OK) {
            while (port_iter_getnext(&pit)) {
                vtss_ifindex_t ifindex;
                if (vtss_ifindex_from_port(sid, pit.iport, &ifindex)  != VTSS_RC_OK) {
                    T_E("Could not get ifindex");
                }

                char receive_port_str[255]       = "";
                (void) lldp_remote_receive_port_to_string(pit.iport, &receive_port_str[0], sid);
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s", receive_port_str);
                cyg_httpd_write_chunked(p->outbuffer, ct);

                for (p_index = LLDPMED_POLICY_MIN ; p_index <= LLDPMED_POLICY_MAX; p_index++ ) {
                    VTSS_RC(vtss_appl_lldp_conf_policy_get(p_index, &policy));
                    if (policy.in_use) {
                        BOOL enabled;
                        if (vtss_appl_lldp_conf_port_policy_get(ifindex, p_index, &enabled) != VTSS_RC_OK) {
                            T_E("Could not get policy");
                        }
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ",%u",
                                      enabled);

                        cyg_httpd_write_chunked(p->outbuffer, ct);
                    }
                }


                cyg_httpd_write_chunked("#", 1);
            }
        }

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%s|",
                      err_msg);
        cyg_httpd_write_chunked(p->outbuffer, ct);

        strcpy(err_msg, ""); // Clear error message

        T_R("cyg_httpd_write_chunked->%s", p->outbuffer);

        cyg_httpd_end_chunked();
    }
    return -1; // Do not further search the file system.
}
#endif // VTSS_SW_OPTION_LLDP_MED

//
// LLDP statistics handler
//
static i32 handler_config_lldp_statistic(CYG_HTTPD_STATE *p)
{
    vtss_isid_t         isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int                 ct;
    char                receive_port[255] = "";
    port_iter_t         pit;
    char                last_changed_str[255] = "";
    mesa_rc             rc;

    //
    // clearing counters
    //
    u64 clear_mask;

    if (!cyg_httpd_form_varable_uint64(p, "clear", &clear_mask)) {
        clear_mask = 0;
    }
    T_I("clear_mask:0x" VPRI64x, clear_mask);

    if (clear_mask) {     /* Clear? */
        if (msg_switch_exists(isid)) {
            //
            // Global counters
            //
            if (clear_mask & ((u64)1 << 49)) { // We use bit 49 for global counters, see lldp_statistics.htm as well
                if ((rc = vtss_appl_lldp_global_stat_clr()) != VTSS_RC_OK) {
                    T_E("%s", error_txt(rc));
                }
            }

            //
            // Interface counters
            //

            // Clear all port counters for this sid
            if ((rc = port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT)) != VTSS_RC_OK) {
                T_E("%s", error_txt(rc));
            }

            while (port_iter_getnext(&pit)) {
                if (clear_mask & ((u64)1 << pit.iport)) {
                    vtss_ifindex_t ifindex;
                    T_I("iport:%d checked", pit.iport);
                    if ((rc = vtss_ifindex_from_port(isid, pit.iport, &ifindex)) != VTSS_RC_OK) {
                        T_E("isid:%d, iport:%d, %s", isid, pit.iport, error_txt(rc));
                    }

                    if ((rc = vtss_appl_lldp_if_stat_clr(ifindex)) != VTSS_RC_OK) {
                        T_E("%s", error_txt(rc));
                    }
                }
            }
        }
    }


    //
    // Getting data
    //

    if (web_get_method(p, VTSS_MODULE_ID_LLDP) == CYG_HTTPD_METHOD_GET) {
        cyg_httpd_start_chunked("html");

        vtss_appl_lldp_global_counters_t global_stat;
        if ((rc = vtss_appl_lldp_stat_global_get(&global_stat)) != VTSS_RC_OK) {
            T_E("%s", error_txt(rc));
        }


        lldp_mgmt_last_change_ago_to_str(global_stat.last_change_ago, &last_changed_str[0]);

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s#%d#%d#%d#%d#|",
                      last_changed_str,
                      global_stat.table_inserts,
                      global_stat.table_deletes,
                      global_stat.table_drops,
                      global_stat.table_ageouts
                     );
        cyg_httpd_write_chunked(p->outbuffer, ct);

#ifdef VTSS_SW_OPTION_AGGR
        // Get statistic for LAGs
        aggr_mgmt_group_member_t glag_members;
        mesa_glag_no_t glag_no;
        for (glag_no = AGGR_MGMT_GROUP_NO_START; glag_no < (AGGR_MGMT_GROUP_NO_START + AGGR_LLAG_CNT_); glag_no++) {
            mesa_port_no_t iport;

            // Get the port members
            (void)aggr_mgmt_members_get(isid, glag_no, &glag_members, FALSE);

            // Determine if there is a port and use the lowest port number for statistic (See packet_api.h as well).
            for (iport = 0; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
                if (glag_members.entry.member[iport]) {
                    strcpy(receive_port, l2port2str(fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) + glag_no - AGGR_MGMT_GROUP_NO_START));

                    vtss_appl_lldp_port_counters_t stati;
                    vtss_ifindex_t ifindex;
                    if (vtss_ifindex_from_port(isid, iport, &ifindex) != VTSS_RC_OK) {
                        T_E("Could not get ifindex");
                    }
                    if (vtss_appl_lldp_stat_if_get(ifindex, &stati)) {
                        T_E("Could not get statistics");
                    }

                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s#%d#%d#%d#%d#%d#%d#%d#%d#|",
                                  receive_port,                                                                         /* henrikb - not sure */
                                  stati.statsFramesOutTotal,
                                  stati.statsFramesInTotal,
                                  stati.statsFramesInErrorsTotal,
                                  stati.statsFramesDiscardedTotal,
                                  stati.statsTLVsDiscardedTotal,
                                  stati.statsTLVsUnrecognizedTotal,
                                  stati.statsOrgTVLsDiscarded,
                                  stati.statsAgeoutsTotal);
                    cyg_httpd_write_chunked(p->outbuffer, ct);
                    break;
                }
            }
        }
#endif

        // Loop through all front ports
        if (port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT) == VTSS_RC_OK) {
            while (port_iter_getnext(&pit)) {
                // Check if port is part of a GLAG
                if (lldp_remote_receive_port_to_string(pit.iport, &receive_port[0], isid) == 1) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s#########|", receive_port);
                } else {
                    vtss_appl_lldp_port_counters_t stati;
                    vtss_ifindex_t ifindex;
                    if (vtss_ifindex_from_port(isid, pit.iport, &ifindex) != VTSS_RC_OK) {
                        T_E("Could not get ifindex");
                    }
                    if (vtss_appl_lldp_stat_if_get(ifindex, &stati)) {
                        T_E("Could not get statistics");
                    }

                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s#%d#%d#%d#%d#%d#%d#%d#%d#|",
                                  receive_port,
                                  stati.statsFramesOutTotal,
                                  stati.statsFramesInTotal,
                                  stati.statsFramesInErrorsTotal,
                                  stati.statsFramesDiscardedTotal,
                                  stati.statsTLVsDiscardedTotal,
                                  stati.statsTLVsUnrecognizedTotal,
                                  stati.statsOrgTVLsDiscarded,
                                  stati.statsAgeoutsTotal);
                }

                cyg_httpd_write_chunked(p->outbuffer, ct);

            } // End pit.iport for loop

            T_R("cyg_httpd_write_chunked -> %s", p->outbuffer);
        }

        cyg_httpd_end_chunked();
    }
    return -1; // Do not further search the file system.
}



#ifdef VTSS_SW_OPTION_EEE
//
// LLDP EEE neighbors handler
//
static i32 handler_lldp_eee_neighbors(CYG_HTTPD_STATE *p)
{
    vtss_isid_t         isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int                 ct;
    BOOL                no_entries_found = TRUE;
    vtss_lldp_entry_index_t entry_index;
    BOOL                insert_seperator = FALSE;
    eee_switch_state_t  eee_state;
    port_iter_t         pit;
    vtss_appl_lldp_cap_t cap;
    if (vtss_appl_lldp_cap_get(&cap) != VTSS_RC_OK) {
        T_E("Could not get capabilities");
    }

    if (web_get_method(p, VTSS_MODULE_ID_LLDP) == CYG_HTTPD_METHOD_GET) {

        if (eee_mgmt_switch_state_get(isid, &eee_state) != VTSS_RC_OK) {
            T_E("Could not get eee state");
            return 0;
        };

        cyg_httpd_start_chunked("html");

        if (port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT) == VTSS_RC_OK) {
            while (port_iter_getnext(&pit)) {
                entry_index = 0;

                vtss_ifindex_t ifindex;
                VTSS_RC(vtss_ifindex_from_port(isid, pit.iport, &ifindex));
                vtss_appl_lldp_remote_entry_t entry;

                while (vtss_appl_lldp_entry_get(ifindex, &entry_index, &entry, TRUE) == VTSS_RC_OK) {
                    entry_index++;
                    no_entries_found = FALSE;
                    if (insert_seperator) {
                        cyg_httpd_write_chunked("|", 1);
                    }
                    insert_seperator = TRUE;

                    eee_switch_conf_t switch_conf;
                    VTSS_RC(eee_mgmt_switch_conf_get(isid, &switch_conf));

                    u8 in_sync = (entry.eee.RemTxTwSysEcho == eee_state.port[pit.iport].LocTxSystemValue) && (entry.eee.RemRxTwSysEcho == eee_state.port[pit.iport].LocRxSystemValue);
                    T_D("in_sync = %d", in_sync);
                    char  receive_port[255] = "";
                    (void) lldp_remote_receive_port_to_string(pit.iport, &receive_port[0], isid);
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s#%d#%d#%d#%d#%d#%d#%d#%d#%d#%d#%d",
                                  receive_port,
                                  entry.eee.RemTxTwSys,
                                  entry.eee.RemRxTwSys,
                                  entry.eee.RemFbTwSys,
                                  entry.eee.RemTxTwSysEcho,
                                  entry.eee.RemRxTwSysEcho,
                                  in_sync,
                                  eee_state.port[pit.iport].LocResolvedTxSystemValue,
                                  eee_state.port[pit.iport].LocResolvedRxSystemValue,
                                  switch_conf.port[pit.iport].eee_ena,
                                  eee_state.port[pit.iport].eee_capable,
                                  eee_state.port[pit.iport].link_partner_eee_capable);
                    cyg_httpd_write_chunked(p->outbuffer, ct);
                }
            } // End iport for loop
        }

        if (no_entries_found) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|",
                          "No EEE info");
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        T_R("cyg_httpd_write_chunked->%s", p->outbuffer);

        cyg_httpd_end_chunked();
    }
    return -1; // Do not further search the file system.
}
#endif

#ifdef VTSS_SW_OPTION_POE
static i32 handler_lldp_poe_neighbors(CYG_HTTPD_STATE *p)
{
    vtss_isid_t           sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int                   ct;
    BOOL                  no_entries_found = true;
    u32                   entry_index;
    vtss_appl_lldp_remote_entry_t   *table = NULL, *entry;
    port_iter_t           pit;

    if (sid != VTSS_ISID_START) {
        return -1;
    }
    if (web_get_method(p, VTSS_MODULE_ID_LLDP) == CYG_HTTPD_METHOD_GET) {

        cyg_httpd_start_chunked("html");

        // Get LLDP information
        vtss_appl_lldp_mutex_lock();
        table = vtss_appl_lldp_entries_get(); // Get the entries table.
        vtss_appl_lldp_cap_t cap;
        if (vtss_appl_lldp_cap_get(&cap) != VTSS_RC_OK) {
            T_E("Could not get capabilities");
        };

        if (port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT) == VTSS_RC_OK) {
            while (port_iter_getnext(&pit)) {

                entry = table;       // Restart the entries
                for (entry_index = 0 ; entry_index < cap.remote_entries_cnt; ++entry_index) {

                    // This do the sorting of the entries ( Sorted by recieved port )
                    if (entry->receive_port != pit.iport || entry->in_use == 0) {
                        entry++; // Get next entry
                        continue;
                    }

                    int power_type     = 0;
                    int power_source   = 0;
                    int power_priority = 0;
                    int power_value    = 0;

                    if (lldp_remote_get_poe_power_info(entry, &power_type, &power_source, &power_priority, &power_value)) {
                        no_entries_found = false; // Signal that at least one entry contained data

                        T_D_PORT(pit.iport, "Entry found, power_value:%d", power_value);
                        // Push data to web
                        char  receive_port[255] = "";
                        (void) lldp_remote_receive_port_to_string(pit.iport, &receive_port[0], sid);
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s#%d#%d#%d#%d#|",
                                      receive_port,
                                      power_type,
                                      power_source,
                                      power_priority,
                                      power_value);

                        cyg_httpd_write_chunked(p->outbuffer, ct);
                    }

                    T_D_PORT(pit.iport, "%d/%d/%d/%d/|",
                             power_type,
                             power_source,
                             power_priority,
                             power_value);



                    entry++;// Get next entry

                } // End iport for loop
            }

            T_R("cyg_httpd_write_chunked -> %s", p->outbuffer);

            if (no_entries_found) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|", "");
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
        cyg_httpd_end_chunked();
        vtss_appl_lldp_mutex_unlock();
    }
    return -1; // Do not further search the file system.
}
#endif //// VTSS_SW_OPTION_POE
//
// LLDP config handler
//

static i32 handler_config_lldp_config(CYG_HTTPD_STATE *p)
{
    vtss_isid_t           sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int                   ct;
    BOOL                  rx_enable = 0, tx_enable = 0;
    int                   form_value;
    char                  form_name[32];
    static char           err_msg[100]; // Need to be static because it is set when posting data, but used getting data.
    mesa_rc               rc = VTSS_RC_OK;
    port_iter_t           pit;

    T_I ("lldpmed_config  web access - SID =  %d", sid);
    if (sid != VTSS_ISID_START) {
        redirect(p, "/lldp_med_config.htm");
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_LLDP)) {
        return -1;
    }
#endif

    if (web_get_method(p, VTSS_MODULE_ID_LLDP) == CYG_HTTPD_METHOD_POST) {

        vtss_appl_lldp_common_conf_t conf;

        strcpy(err_msg, ""); // No errors so far :)
        //
        // Parameters
        //
        (void) vtss_appl_lldp_common_conf_get(&conf); // Get current configuration

        // Get tx interval from WEB ( Form name = "txInterval" )
        if (cyg_httpd_form_varable_int(p, "txInterval", &form_value)) {
            T_D ("txinterval set to %d via web", form_value);
            conf.tx_sm.msgTxInterval = form_value;
        }

        if (cyg_httpd_form_varable_int(p, "txHold", &form_value)) {
            T_D ("txhold set to %d via web", form_value);
            conf.tx_sm.msgTxHold  = form_value;
        }

        if (cyg_httpd_form_varable_int(p, "txDelay", &form_value)) {
            T_D ("txdelay set to %d via web", form_value);
            conf.tx_sm.txDelay = form_value;
        }

        if (cyg_httpd_form_varable_int(p, "reInitDelay", &form_value)) {
            conf.tx_sm.reInitDelay = form_value;
        }

        // Update the configuration
        if ((rc = vtss_appl_lldp_common_conf_set(&conf)) != VTSS_RC_OK) {
            goto stop;
        }

        //
        // Admin states
        //
        if ((rc = port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT)) == VTSS_RC_OK) {
            CapArray<vtss_appl_lldp_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_conf;
            (void) lldp_mgmt_conf_get(&port_conf[0]); // Get current configuration
            T_I("sid:%d", sid);
            while (port_iter_getnext(&pit)) {

                sprintf(form_name, "mode_%u", pit.uport); // Set form name
                if (cyg_httpd_form_varable_int(p, form_name, &form_value)   && (form_value >= 0 && form_value < 4)) {
                    // form_value ok
                } else {
                    T_E("Unknown value for form : %s, value = %d disabling LLDP ", form_name, form_value);
                    form_value = 0;
                }

                T_N("uport = %u, form_value = %d", pit.uport, form_value);
                if (form_value == 0) {
                    T_N("uport = %u disabled", pit.uport);
                    port_conf[pit.iport].admin_states = VTSS_APPL_LLDP_DISABLED;
                } else if (form_value == 2) {
                    port_conf[pit.iport].admin_states = VTSS_APPL_LLDP_ENABLED_RX_ONLY;
                } else if (form_value == 1) {
                    port_conf[pit.iport].admin_states = VTSS_APPL_LLDP_ENABLED_TX_ONLY;
                } else {
                    // form_value is 3
                    port_conf[pit.iport].admin_states = VTSS_APPL_LLDP_ENABLED_RX_TX;
                }


#ifdef VTSS_SW_OPTION_CDP
                //
                // CDP AWARE
                //
                sprintf(form_name, "cdp_aware_%u", pit.uport); // Set form name
                if (cyg_httpd_form_varable_find(p, form_name) ) {
                    port_conf[pit.iport].cdp_aware = 1;
                } else {
                    port_conf[pit.iport].cdp_aware = 0;
                }
#endif

                //
                // Trap
                //
                sprintf(form_name, "trap_ena_%u", pit.uport); // Set form name
                if (cyg_httpd_form_varable_find(p, form_name) ) {
                    port_conf[pit.iport].snmp_notification_ena = TRUE;
                } else {
                    port_conf[pit.iport].snmp_notification_ena = FALSE;
                }

                //
                // Configure  optional TLVs
                //
                // Port description
                sprintf(form_name, "port_descr_ena_%u", pit.uport); // Set to the htm checkbox form name
                T_D("pit.uport:%u, pit.iport:%u,  form_name:%s value:%s", pit.uport, pit.iport, form_name, cyg_httpd_form_varable_find(p, form_name));
                if (cyg_httpd_form_varable_find(p, form_name)) {
                    port_conf[pit.iport].optional_tlvs_mask |= VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_PORT_DESCR_BIT;
                } else {
                    port_conf[pit.iport].optional_tlvs_mask &= ~VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_PORT_DESCR_BIT;
                }

                // sys_name
                sprintf(form_name, "sys_name_ena_%u", pit.uport); // Set to the htm checkbox form name
                if (cyg_httpd_form_varable_find(p, form_name) ) {
                    port_conf[pit.iport].optional_tlvs_mask |= VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_NAME_BIT;
                } else {
                    port_conf[pit.iport].optional_tlvs_mask &= ~VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_NAME_BIT;
                }

                // sys_descr
                sprintf(form_name, "sys_descr_ena_%u", pit.uport); // Set to the htm checkbox form name
                if (cyg_httpd_form_varable_find(p, form_name) ) {
                    port_conf[pit.iport].optional_tlvs_mask |= VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_DESCR_BIT;
                } else {
                    port_conf[pit.iport].optional_tlvs_mask &= ~VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_DESCR_BIT;
                }

                // sys_capa
                sprintf(form_name, "sys_capa_ena_%u", pit.uport); // Set to the htm checkbox form name
                if (cyg_httpd_form_varable_find(p, form_name) ) {
                    port_conf[pit.iport].optional_tlvs_mask |= VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_CAPA_BIT;
                } else {
                    port_conf[pit.iport].optional_tlvs_mask &= ~VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_CAPA_BIT;
                }

                // management addr
                sprintf(form_name, "mgmt_addr_ena_%u", pit.uport); // Set to the htm checkbox form name
                if (cyg_httpd_form_varable_find(p, form_name) ) {
                    port_conf[pit.iport].optional_tlvs_mask |= VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_ADDR_BIT;
                } else {
                    port_conf[pit.iport].optional_tlvs_mask &= ~VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_ADDR_BIT;
                }
            }
            (void) lldp_mgmt_conf_set(&port_conf[0]);

        }

stop:
        if (rc != VTSS_RC_OK) {
            misc_strncpyz(err_msg, error_txt(rc), 100); // Set err_msg ( if any )
        }
        redirect(p, "/lldp_config.htm");

    } else if (web_get_method(p, VTSS_MODULE_ID_LLDP) == CYG_HTTPD_METHOD_GET) {
        //
        // Getting the configuration.
        //

        cyg_httpd_start_chunked("html");

        // Apply error message (If any )
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|", err_msg);
        cyg_httpd_write_chunked(p->outbuffer, ct);
        strcpy(err_msg, ""); // Clear error message


        vtss_appl_lldp_common_conf_t conf;
        (void) vtss_appl_lldp_common_conf_get(&conf) ;


        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d#%d#%d#%d|",
                      conf.tx_sm.msgTxInterval,
                      conf.tx_sm.msgTxHold,
                      conf.tx_sm.txDelay,
                      conf.tx_sm.reInitDelay);

        cyg_httpd_write_chunked(p->outbuffer, ct);


        if (port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT) == VTSS_RC_OK) {
            CapArray<vtss_appl_lldp_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_conf;
            (void) lldp_mgmt_conf_get(&port_conf[0]); // Get current configuration

            while (port_iter_getnext(&pit)) {
                if (port_conf[pit.iport].admin_states == VTSS_APPL_LLDP_ENABLED_RX_TX ) {
                    tx_enable = 1;
                    rx_enable = 1;
                } else if (port_conf[pit.iport].admin_states == VTSS_APPL_LLDP_ENABLED_TX_ONLY ) {
                    tx_enable = 1;
                    rx_enable = 0;
                } else if (port_conf[pit.iport].admin_states == VTSS_APPL_LLDP_ENABLED_RX_ONLY ) {
                    tx_enable = 0;
                    rx_enable = 1;
                } else {
                    // LLDP disabled
                    tx_enable = 0;
                    rx_enable = 0;
                }

                BOOL trap_ena = port_conf[pit.iport].snmp_notification_ena;
                //
                // Getting if the optional TLVs are enabled
                //
                BOOL port_descr_ena = 0;
                BOOL sys_name_ena   = 0;
                BOOL sys_descr_ena  = 0;
                BOOL sys_capa_ena   = 0;
                BOOL mgmt_addr_ena  = 0;

                port_descr_ena = port_conf[pit.iport].optional_tlvs_mask & VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_PORT_DESCR_BIT ? TRUE : FALSE;
                sys_name_ena   = port_conf[pit.iport].optional_tlvs_mask & VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_NAME_BIT ? TRUE : FALSE;
                sys_descr_ena  = port_conf[pit.iport].optional_tlvs_mask & VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_DESCR_BIT ? TRUE : FALSE;
                sys_capa_ena   = port_conf[pit.iport].optional_tlvs_mask & VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_CAPA_BIT ? TRUE : FALSE;
                mgmt_addr_ena  = port_conf[pit.iport].optional_tlvs_mask & VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_ADDR_BIT ? TRUE : FALSE;

                //
                // Pass configuration to Web
                //
                char  if_str[255] = "";
                (void) lldp_remote_receive_port_to_string(pit.iport, &if_str[0], sid);

                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d#%s#%d#%d#%d#%d#%d#%d#%d#%d#%d,",
                              pit.uport,
                              if_str,
                              tx_enable,
                              rx_enable,
#ifdef VTSS_SW_OPTION_CDP
                              port_conf[pit.iport].cdp_aware,
#else
                              0,
#endif
                              trap_ena,
                              port_descr_ena,
                              sys_name_ena,
                              sys_descr_ena,
                              sys_capa_ena,
                              mgmt_addr_ena
                             );

                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }

        T_R("cyg_httpd_write_chunked -> %s", p->outbuffer);

        cyg_httpd_end_chunked();
    }
    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/

static size_t lldp_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[LLDP_WEB_BUF_LEN];
    (void) snprintf(buff, LLDP_WEB_BUF_LEN,
#ifdef VTSS_SW_OPTION_LLDP_MED
                    "var configLldpmedPoliciesMin = %u;\n"
                    "var configLldpmedPoliciesMax = %u;\n"
#endif
                    "var configLLDPRemoteEntriesMax = \"%d\";\n"
#ifdef VTSS_SW_OPTION_CDP
                    "var configHasCDP = 1;\n"
#else
                    "var configHasCDP = 0;\n"
#endif
#ifdef VTSS_SW_OPTION_POE
                    "var configHasPoE = 1;\n",
#else
                    "var configHasPoE = 0;\n",
#endif
#ifdef VTSS_SW_OPTION_LLDP_MED
                    LLDPMED_POLICY_MIN, LLDPMED_POLICY_MAX,
#endif
                    LLDP_REMOTE_ENTRIES
                   );
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  JS lib config table entry                                               */
/****************************************************************************/
web_lib_config_js_tab_entry(lldp_lib_config_js);
/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_lldp_neighbor, "/config/lldp_neighbors", handler_config_lldp_neighbor);
#ifdef VTSS_SW_OPTION_LLDP_MED
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_lldp_neighbor_med, "/stat/lldpmed_neighbors", handler_lldp_neighbor_med);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_lldpmed_config, "/config/lldpmed_config", handler_lldpmed_config);
#endif /* VTSS_SW_OPTION_LLDP_MED */
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_lldp_statistic, "/config/lldp_statistics", handler_config_lldp_statistic);
#ifdef VTSS_SW_OPTION_POE
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_lldp_poe_neighbors, "/stat/lldp_poe_neighbors", handler_lldp_poe_neighbors);
#endif /* VTSS_SW_OPTION_POE */
#ifdef VTSS_SW_OPTION_EEE
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_lldp_eee_neighbors, "/stat/lldp_eee_neighbors", handler_lldp_eee_neighbors);
#endif /* VTSS_SW_OPTION_POE */

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_lldp_config, "/config/lldp_config", handler_config_lldp_config);

