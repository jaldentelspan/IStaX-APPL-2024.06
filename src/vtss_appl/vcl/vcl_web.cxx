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

#include "web_api.h"
#include "vcl_api.h"
#include "port_iter.hxx" /* For port_iter_t */

#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif
/* =================
 * Trace definitions
 * -------------- */
//#include <vtss_trace_lvl_api.h>
//#include <vtss_trace_api.h>
#include "vcl_trace.h"
/* ============== */

#define VCL_WEB_BUF_LEN 512

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/
static i32 handler_config_vcl_mac_based_conf(CYG_HTTPD_STATE *p)
{
    vcl_mac_mgmt_vce_conf_local_t  entry;
    char                           search_str[32];
    const char                     *str;
    vtss_isid_t                    sid  = web_retrieve_request_sid(p); /* Includes USID = ISID */
    size_t                         len;
    char                           str_buff[24];
    vcl_mac_mgmt_vce_conf_global_t gentry;
    mesa_mac_t                     mac_addr;
    uint                           temp_mac[6];
    port_iter_t                    pit;
    i32                            vid_errors = 0;
    mesa_rc                        rc;
    char                           newstr[6] = {0};
    BOOL                           first = TRUE, next = FALSE, found_ports = FALSE;
    mesa_port_list_t               ports;
    int                            i, k, vid, mask_port = 0;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }
#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VCL)) {
        return -1;
    }
#endif
    if (p->method == CYG_HTTPD_METHOD_POST) {
        int  new_vid, new_entry;
        // We need to know how many entries that is delete when that save button is pressed to
        // make sure that we do not loop through more entries than the number of valid entries shown
        // at the web page.
        i = 0; //index variable
        memset(search_str, 0, sizeof(search_str));
        T_DG(TRACE_GRP_WEB, "WEB - Deleting checked (old) entries");
        while (i < VCL_MAC_VCE_MAX) {
            sprintf(search_str, "delete_%d", ++i);
            if (cyg_httpd_form_varable_find(p, search_str)) {//fine and delete
                sprintf(search_str, "hiddenmac_%d", i);
                if ((str = cyg_httpd_form_varable_string(p, search_str, &len)) != NULL) {
                    if (sscanf(str, "%2x-%2x-%2x-%2x-%2x-%2x", &temp_mac[0], &temp_mac[1], &temp_mac[2],
                               &temp_mac[3], &temp_mac[4], &temp_mac[5]) == 6) {
                        for (k = 0; k < 6; k++) {
                            mac_addr.addr[k] = temp_mac[k];
                        }
                        if ((rc = vcl_mac_mgmt_conf_del(sid, &mac_addr)) != VTSS_RC_OK) {
                            T_D("Deletion failed\n");
                            if (rc == VCL_ERROR_ENTRY_NOT_FOUND) {
                                redirect(p, "/mac_based_vlan.htm?MAC_error=3");
                                return -1;
                            }
                        }
                    }
                }
            }
        }
        T_DG(TRACE_GRP_WEB, "WEB - Deleted checked (old) entries successfully");

        memset(search_str, 0, sizeof(search_str));
        /* Update the existing entries if ports are changed */
        T_DG(TRACE_GRP_WEB, "WEB - Editing updated (old) entries");
        for (i = 1; i <= VCL_MAC_VCE_MAX; i++) {
            /* For deleted entries, don't check for updates */
            sprintf(search_str, "delete_%d", i);
            if (cyg_httpd_form_varable_find(p, search_str)) {
                continue;
            }
            sprintf(search_str, "hiddenmac_%d", i);
            if ((str = cyg_httpd_form_varable_string(p, search_str, &len)) != NULL) {
                memset(&mac_addr, 0, sizeof(mac_addr));
                vtss_clear(entry);
                vtss_clear(gentry);
                memset(ports, 0, sizeof(ports));
                if (sscanf(str, "%2x-%2x-%2x-%2x-%2x-%2x", &temp_mac[0], &temp_mac[1], &temp_mac[2],
                           &temp_mac[3], &temp_mac[4], &temp_mac[5]) == 6) {
                    for (k = 0; k < 6; k++) {
                        mac_addr.addr[k] = temp_mac[k];
                    }
                }
                gentry.smac = mac_addr;
                if (!cyg_httpd_form_variable_int_fmt(p, &vid, "hiddenvid_%d", i)) { /* Get VLAN ID */
                    continue; /* This should never happen */
                }
                T_NG(TRACE_GRP_WEB, "WEB - Editing old entry with MAC %02x:%02x:%02x:%02x:%02x:%02x and VID %u",
                     temp_mac[0], temp_mac[1], temp_mac[2], temp_mac[3], temp_mac[4], temp_mac[5], vid);
                mask_port = 0;
                (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                while (port_iter_getnext(&pit)) {
                    mask_port++;
                    sprintf(search_str, "mask_%d_%d", i, mask_port);
                    if (cyg_httpd_form_varable_find(p, search_str)) { /* "on" if checked */
                        ports[pit.iport] = TRUE;
                        found_ports = TRUE;
                    }
                }
                if (found_ports == FALSE) {
                    redirect(p, "/mac_based_vlan.htm?MAC_error=4");
                    return -1;
                }
                if ((vcl_mac_mgmt_conf_get(sid, &gentry, FALSE, FALSE) == VTSS_RC_OK) && (!memcmp(&gentry.smac, &mac_addr, sizeof(mac_addr)))) {
                    entry.smac = gentry.smac;
                    entry.vid = gentry.vid;
                    memcpy(entry.ports, gentry.ports[sid - VTSS_ISID_START], sizeof(entry.ports));
                    /* If VID and Ports are not changed, we need not update the entry */
                    if ((entry.vid != vid) || (memcmp(ports, entry.ports, sizeof(ports)))) {
                        entry.smac = mac_addr;
                        entry.vid = vid;
                        memcpy(entry.ports, ports, sizeof(ports));
                        if ((rc = vcl_mac_mgmt_conf_add(sid, &entry)) != VTSS_RC_OK) {
                            if (rc == VCL_ERROR_ENTRY_DIFF_VID) {
                                vid_errors++;
                            }
                            T_WG(TRACE_GRP_WEB, "WEB - %s", error_txt(rc));
                        }
                    }
                }
            }
        }
        if (vid_errors == 0) {
            T_DG(TRACE_GRP_WEB, "WEB - Edited updated (old) entries successfully");
        }
        new_entry = 0;//while loop index variable
        T_DG(TRACE_GRP_WEB, "WEB - Adding new entries");
        for (i = 1; i <= VCL_MAC_VCE_MAX; i++) {
            new_entry++;
            /* Add new entry */
            new_vid = new_entry;
            sprintf(search_str, "vid_new_%d", new_vid);
            if (cyg_httpd_form_varable_int(p, search_str, &new_vid) && new_vid > VTSS_VID_NULL && new_vid < VTSS_VIDS) {
                entry.vid = 0xfff & (uint)new_vid;
                (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                while (port_iter_getnext(&pit)) {
                    sprintf(search_str, "mask_new_%d_%d", new_entry, pit.uport);
                    entry.ports[pit.iport] = FALSE;
                    if (cyg_httpd_form_varable_find(p, search_str)) { /* "on" if checked */
                        entry.ports[pit.iport] = TRUE;
                    }
                }
                sprintf(search_str, "MACID_new_%d", new_entry);
                if ((str = cyg_httpd_form_varable_string(p, search_str, &len)) != NULL) {
                    strncpy(newstr, str, 4);
                    memset(temp_mac, 0, sizeof(temp_mac));
                    if (strchr(newstr, '-')) {
                        if (sscanf(str, "%02x-%02x-%02x-%02x-%02x-%02x", &temp_mac[0], &temp_mac[1], &temp_mac[2],
                                   &temp_mac[3], &temp_mac[4], &temp_mac[5]) == 6) {
                            for (k = 0; k < 6; k++) {
                                entry.smac.addr[k] = (uchar) temp_mac[k];
                            }
                        }
                    } else if (strchr(newstr, '.')) {
                        if (sscanf(str, "%02x.%02x.%02x.%02x.%02x.%02x", &temp_mac[0], &temp_mac[1], &temp_mac[2],
                                   &temp_mac[3], &temp_mac[4], &temp_mac[5]) == 6) {
                            for (k = 0; k < 6; k++) {
                                entry.smac.addr[k] = (uchar) temp_mac[k];
                            }
                        }
                    } else if (isxdigit(newstr[2])) {
                        if (sscanf(str, "%02x%02x%02x%02x%02x%02x", &temp_mac[0], &temp_mac[1], &temp_mac[2],
                                   &temp_mac[3], &temp_mac[4], &temp_mac[5]) == 6) {
                            for (k = 0; k < 6; k++) {
                                entry.smac.addr[k] = (uchar) temp_mac[k];
                            }
                        }
                    } else {
                        redirect(p, "/mac_based_vlan.htm?MAC_error=2");
                        return -1;
                    }
                }
                rc = vcl_mac_mgmt_conf_add(sid, &entry);
                if (rc == VCL_ERROR_ENTRY_DIFF_VID) {
                    vid_errors++;
                }
                if (rc != VTSS_RC_OK) {
                    T_DG(TRACE_GRP_WEB, "WEB - %s", error_txt(rc));
                }
            }
        }
        if (vid_errors == 0) {
            T_DG(TRACE_GRP_WEB, "WEB - Added new entries successfully");
        }
        redirect(p, vid_errors ? "/mac_based_vlan.htm?MAC_error=1" : "/mac_based_vlan.htm");
        return -1;
    } else { /* GET Method */
        i32 ct;
        T_DG(TRACE_GRP_WEB, "WEB - Refreshing entry table");
        cyg_httpd_start_chunked("html");
        while (vcl_mac_mgmt_conf_get(sid, &gentry, first, next) == VTSS_RC_OK) {
            entry.smac = gentry.smac;
            entry.vid = gentry.vid;
            memcpy(entry.ports, gentry.ports[sid - 1], sizeof(entry.ports));
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%s/%u/",
                          sid,
                          (misc_mac_txt(entry.smac.addr, str_buff)),
                          entry.vid);
            cyg_httpd_write_chunked(p->outbuffer, ct);
            /* Send member ports list for current entry */
            (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_ALL);
            while (port_iter_getnext(&pit)) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,", entry.ports.get(pit.iport));
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
            first = FALSE;
            next = TRUE;
            cyg_httpd_write_chunked(";", 1);
        }
        cyg_httpd_end_chunked();
        T_DG(TRACE_GRP_WEB, "WEB - Refreshed entry table");
        return -1;
    }
}
static i32 handler_config_vcl_proto2grp_map(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                       sid  = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vcl_proto_mgmt_group_conf_proto_t group_conf;
    const char                        *str;
    char                              search_str[32];
    size_t                            len;
    mesa_rc                           rc = VTSS_RC_OK;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }
#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VCL)) {
        return -1;
    }
#endif
    if (p->method == CYG_HTTPD_METHOD_POST) {/* POST Method */
        int  proto_errors = 0, temp, temp_value, j;
        ulong temp_ulong_value = 0;
        uint i;
        uint oui[OUI_SIZE];

        i = 0; /* index variable */
        memset(search_str, 0, sizeof(search_str));
        memset(&group_conf, 0, sizeof(group_conf));
        /* Delete the entries with delete check button checked */
        T_DG(TRACE_GRP_WEB, "WEB - Deleting checked (old) entries");
        for (i = 1; i <= VCL_PROTO_PROTOCOL_MAX; i++) {
            sprintf(search_str, "delete_%d", i);
            memset(&group_conf, 0, sizeof(group_conf));
            if (cyg_httpd_form_varable_find(p, search_str)) {//find and delete
                sprintf(search_str, "hiddenvalue_%d", i);
                if ((str = cyg_httpd_form_varable_string(p, search_str, &len)) != NULL) {
                    if (len == 4) {/* Frame Type is Ethernet */
                        group_conf.proto_encap_type = VTSS_APPL_VCL_PROTO_ENCAP_ETH2;
                        if (sscanf(str, "%4x", &temp) == 1) {
                            group_conf.proto.eth2_proto.eth_type = temp;
                        }
                    } else if (len == 5) {/* Frame Type is LLC */
                        group_conf.proto_encap_type = VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER;
                        if (sscanf(str, "%2x-%2x", &temp, &temp_value) == 2) {
                            group_conf.proto.llc_other_proto.dsap = temp;
                            group_conf.proto.llc_other_proto.ssap = temp_value;
                        }
                    } else if (len == 13) {/* Frame Type is SNAP */
                        group_conf.proto_encap_type = VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP;
                        if (sscanf(str, "%2x-%2x-%2x-%4x", &oui[0], &oui[1], &oui[2], &temp_value) == 4) {
                            for (j = 0; j < 3; j++) {
                                group_conf.proto.llc_snap_proto.oui[j] = oui[j];
                            }
                            group_conf.proto.llc_snap_proto.pid = temp_value;
                        }
                    } else {/* Invalid Frame Type value length */
                        T_D("Invalid Frame Type detected!\n");
                    }
                    if ((rc = vcl_proto_mgmt_proto_del(&group_conf)) != VTSS_RC_OK) {
                        T_D("Protocol to Group mapping Deletion failed\n");
                        if (rc == VCL_ERROR_ENTRY_NOT_FOUND) {
                            redirect(p, "/vcl_protocol_grp_map.htm?PROTO_error=2");
                            return -1;
                        }
                        redirect(p, "/vcl_protocol_grp_map.htm?PROTO_error=1");
                        return -1;
                    }
                }
            }
        }
        T_DG(TRACE_GRP_WEB, "WEB - Deleted checked (old) entries successfully");
        memset(search_str, 0, sizeof(search_str));
        /* Add new entries */
        T_DG(TRACE_GRP_WEB, "WEB - Adding new entries");
        for (i = 1; i <= VCL_PROTO_PROTOCOL_MAX; i++) {
            if (cyg_httpd_form_variable_int_fmt(p, &temp, "Ftype_new_%u", i)) {//find and add
                memset(&group_conf, 0, sizeof(group_conf));
                group_conf.proto_encap_type = (vtss_appl_vcl_proto_encap_type_t)temp;
                if (temp == VTSS_APPL_VCL_PROTO_ENCAP_ETH2) {
                    sprintf(search_str, "value1_E_%d", i);
                    if (cyg_httpd_form_varable_hex(p, search_str, &temp_ulong_value)) {
                        group_conf.proto.eth2_proto.eth_type = temp_ulong_value;
                    }
                } else if (temp == VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP) {
                    sprintf(search_str, "value1_S_%d", i);
                    if ((str = cyg_httpd_form_varable_string(p, search_str, &len)) != NULL) {
                        if (sscanf(str, "%2x-%2x-%2x", &oui[0], &oui[1], &oui[2]) == 3) {
                            for (j = 0; j < 3; j++) {
                                group_conf.proto.llc_snap_proto.oui[j] = oui[j];
                            }
                        }
                    }
                    sprintf(search_str, "value2_S_%d", i);
                    if (cyg_httpd_form_varable_hex(p, search_str, &temp_ulong_value)) {
                        group_conf.proto.llc_snap_proto.pid = temp_ulong_value;
                    }
                } else if (temp == VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER) {
                    sprintf(search_str, "value1_L_%d", i);
                    if (cyg_httpd_form_varable_hex(p, search_str, &temp_ulong_value)) {
                        group_conf.proto.llc_other_proto.dsap = temp_ulong_value;
                    }
                    sprintf(search_str, "value2_L_%d", i);
                    if (cyg_httpd_form_varable_hex(p, search_str, &temp_ulong_value)) {
                        group_conf.proto.llc_other_proto.ssap = temp_ulong_value;
                    }

                } else {
                    T_WG(TRACE_GRP_WEB, "Invalid Frame Type detected!\n");
                    redirect(p, "/vcl_protocol_grp_map.htm?PROTO_error=3");
                    return -1;
                }
                sprintf(search_str, "name_new_%d", i);
                if ((str = cyg_httpd_form_varable_string(p, search_str, &len)) != NULL) {
                    memcpy(group_conf.name, str, len);
                    group_conf.name[len] = '\0';
                    if ((rc = vcl_proto_mgmt_proto_add(&group_conf)) != VTSS_RC_OK) {
                        proto_errors++;
                        T_WG(TRACE_GRP_WEB, "WEB - %s", error_txt(rc));
                    }
                }
            }
        }
        if (proto_errors == 0) {
            T_DG(TRACE_GRP_WEB, "WEB - Added new entries successfully");
        }
        redirect(p, proto_errors ? "/vcl_protocol_grp_map.htm?PROTO_error=4" : "/vcl_protocol_grp_map.htm");
        return -1;
    } else {/* GET Method */
        /* Format: <Frame Type>/<value>/<Group Name>|... */
        BOOL first, next;
        int  ct;
        T_DG(TRACE_GRP_WEB, "WEB - Refreshing entry table");
        cyg_httpd_start_chunked("html");
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "Ethernet#SNAP#LLC,");
        cyg_httpd_write_chunked(p->outbuffer, ct);
        first = TRUE;
        next = FALSE;
        memset(&group_conf, 0, sizeof(group_conf));
        while (vcl_proto_mgmt_proto_get(&group_conf, first, next) == VTSS_RC_OK) {
            first = FALSE;
            next = TRUE;
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/",
                          group_conf.proto_encap_type);
            cyg_httpd_write_chunked(p->outbuffer, ct);
            if (group_conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_ETH2) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%04x/",
                              group_conf.proto.eth2_proto.eth_type);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            } else if (group_conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%02x-%02x-%02x-%04x/",
                              group_conf.proto.llc_snap_proto.oui[0],
                              group_conf.proto.llc_snap_proto.oui[1],
                              group_conf.proto.llc_snap_proto.oui[2],
                              group_conf.proto.llc_snap_proto.pid);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            } else if (group_conf.proto_encap_type == VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%02x-%02x/",
                              group_conf.proto.llc_other_proto.dsap,
                              group_conf.proto.llc_other_proto.ssap);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            } else {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "Invalid");
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|", group_conf.name);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();/* end of http responseText */
        T_DG(TRACE_GRP_WEB, "WEB - Refreshed entry table");
    }
    return -1;
}/* vcl protocol to group mapping configuration handler */

static i32 handler_config_vcl_grp2vlan_map(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                              isid  = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vcl_proto_mgmt_group_conf_entry_local_t  entry;
    vcl_proto_mgmt_group_conf_entry_global_t gentry;
    BOOL                                     first, next;
    const char                               *str;
    char                                     search_str[32];
    size_t                                   len;
    port_iter_t                              pit;
    u32                                      mask_port = 0;
    mesa_port_list_t                         ports;
    i32                                      vid_errors = 0;
    mesa_rc                                  rc = VTSS_RC_OK;

    if (redirectUnmanagedOrInvalid(p, isid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }
#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VCL)) {
        return -1;
    }
#endif
    if (p->method == CYG_HTTPD_METHOD_POST) {/* POST Method */
        int group_errors = 0;
        int temp, i = 0;
        BOOL found_ports = FALSE;
        memset(search_str, 0, sizeof(search_str));
        /* Delete the entries with delete check button checked */
        T_DG(TRACE_GRP_WEB, "WEB - Deleting checked (old) entries");
        for (i = 1; i <= VCL_PROTO_VCE_MAX; i++) {
            sprintf(search_str, "delete_%d", i);
            if (cyg_httpd_form_varable_find(p, search_str)) {
                T_DG(TRACE_GRP_WEB, "WEB - found delete");
                sprintf(search_str, "hiddenGrp_%u", i);
                if ((str = cyg_httpd_form_varable_string(p, search_str, &len)) != NULL) {
                    T_DG(TRACE_GRP_WEB, "WEB - found name %s", str);
                    vtss_clear(entry);
                    memcpy(entry.name, str, len);
                    entry.name[len] = '\0';
                    if (cyg_httpd_form_variable_int_fmt(p, &temp, "hiddenVID_%u", i)) {
                        T_DG(TRACE_GRP_WEB, "WEB - found vid %d", temp);
                        entry.vid = temp;
                        if ((rc = vcl_proto_mgmt_conf_del(isid, &entry)) != VTSS_RC_OK) {
                            T_WG(TRACE_GRP_WEB, "WEB - %s", error_txt(rc));
                            if (rc == VCL_ERROR_ENTRY_NOT_FOUND) {
                                redirect(p, "/vcl_grp_2_vlan_mapping.htm?GROUP_error=2");
                                return -1;
                            }
                            redirect(p, "/vcl_grp_2_vlan_mapping.htm?GROUP_error=1");
                            return -1;
                        }
                    }
                }
            }
        }
        T_DG(TRACE_GRP_WEB, "WEB - Deleted checked (old) entries successfully");
        memset(search_str, 0, sizeof(search_str));
        /* Update the existing entries if ports are changed */
        T_DG(TRACE_GRP_WEB, "WEB - Editing updated (old) entries");
        for (i = 1; i <= VCL_PROTO_VCE_MAX; i++) {
            /* For deleted entries, don't check for updates */
            sprintf(search_str, "delete_%d", i);
            if (cyg_httpd_form_varable_find(p, search_str)) {
                continue;
            }
            sprintf(search_str, "hiddenGrp_%u", i);
            if ((str = cyg_httpd_form_varable_string(p, search_str, &len)) != NULL) {
                vtss_clear(entry);
                vtss_clear(gentry);
                memset(ports, 0, sizeof(ports));
                memcpy(gentry.name, str, len);
                gentry.name[len] = '\0';
                memcpy(entry.name, gentry.name, sizeof(gentry.name));
                if (cyg_httpd_form_variable_int_fmt(p, &temp, "hiddenVID_%u", i)) {
                    gentry.vid = temp;
                    T_NG(TRACE_GRP_WEB, "WEB - Editing old entry with Group name %s and VID %u", str, temp);
                    mask_port = 0;
                    (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                    while (port_iter_getnext(&pit)) {
                        mask_port++;
                        sprintf(search_str, "mask_%d_%d", i, mask_port);
                        if (cyg_httpd_form_varable_find(p, search_str)) { /* "on" if checked */
                            ports[pit.iport] = TRUE;
                            found_ports = TRUE;
                        }
                    }
                    if (found_ports == FALSE) {
                        redirect(p, "/vcl_grp_2_vlan_mapping.htm?GROUP_error=5");
                        return -1;
                    }
                    if ((vcl_proto_mgmt_conf_get(isid, &gentry, FALSE, FALSE) == VTSS_RC_OK) &&
                        (!memcmp(gentry.name, entry.name, sizeof(gentry.name))) && (gentry.vid == temp)) {
                        T_NG(TRACE_GRP_WEB, "WEB - Found entry to edit");
                        memcpy(entry.name, gentry.name, sizeof(gentry.name));
                        entry.vid = gentry.vid;
                        memcpy(entry.ports, gentry.ports[isid - VTSS_ISID_START], sizeof(entry.ports));
                        /* If VID and Ports are not changed, we need not update the entry */
                        if ((memcmp(ports, entry.ports, sizeof(ports)))) {
                            memcpy(entry.name, str, len);
                            entry.name[len] = '\0';
                            entry.vid = temp;
                            memcpy(entry.ports, ports, sizeof(ports));
                            if ((rc = vcl_proto_mgmt_conf_add(isid, &entry)) != VTSS_RC_OK) {
                                group_errors++;
                                T_WG(TRACE_GRP_WEB, "WEB - %s", error_txt(rc));
                            }
                        }
                    }
                }
            }
        }
        if (group_errors == 0) {
            T_DG(TRACE_GRP_WEB, "WEB - Edited updated (old) entries successfully");
        } else {
            redirect(p, "/vcl_grp_2_vlan_mapping.htm?GROUP_error=3");
            return -1;
        }
        /* Add new entries */
        T_DG(TRACE_GRP_WEB, "WEB - Adding new entries");
        for (i = 1; i <= VCL_PROTO_VCE_MAX; i++) {
            sprintf(search_str, "name_new_%u", i);
            if ((str = cyg_httpd_form_varable_string(p, search_str, &len)) != NULL) {
                vtss_clear(entry);
                memcpy(entry.name, str, len);
                entry.name[len] = '\0';
                if (cyg_httpd_form_variable_int_fmt(p, &temp, "vid_new_%u", i)) {
                    entry.vid = temp;
                    mask_port = 0;
                    (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                    while (port_iter_getnext(&pit)) {
                        mask_port++;
                        sprintf(search_str, "mask_new_%d_%d", i, mask_port);
                        if (cyg_httpd_form_varable_find(p, search_str)) { /* "on" if checked */
                            entry.ports[pit.iport] = TRUE;
                        }
                    }
                    if ((rc = vcl_proto_mgmt_conf_add(isid, &entry)) != VTSS_RC_OK) {
                        T_DG(TRACE_GRP_WEB, "WEB - %s", error_txt(rc));
                        if (rc == VCL_ERROR_ENTRY_DIFF_VID) {
                            vid_errors++;
                        } else {
                            redirect(p, "/vcl_grp_2_vlan_mapping.htm?GROUP_error=4");
                            return -1;
                        }
                    }
                }
            }
        }
        if (vid_errors == 0) {
            T_DG(TRACE_GRP_WEB, "WEB - Added new entries successfully");
        }
        redirect(p, vid_errors ?  "/vcl_grp_2_vlan_mapping.htm?GROUP_error=6" : "/vcl_grp_2_vlan_mapping.htm");
        return -1;
    } else { /* GET Method */
        /* Format: <Group Name 1>#<Group Name 2>#...#<Group Name n>,<Group Name>/<VID>/<Member port 1 status>#...<Member port max status> */
        int ct;
        T_DG(TRACE_GRP_WEB, "WEB - Refreshing entry table");
        cyg_httpd_start_chunked("html");
        /* send list of group to VLAN mapping entries */
        first = TRUE;
        next = FALSE;
        vtss_clear(gentry);
        while ((vcl_proto_mgmt_conf_get(isid, &gentry, first, next)) == VTSS_RC_OK) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s/%u/", gentry.name, gentry.vid);
            cyg_httpd_write_chunked(p->outbuffer, ct);
            /* Send member ports list for current entry */
            (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_ALL);
            while (port_iter_getnext(&pit)) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,", gentry.ports[isid - VTSS_ISID_START].get(pit.iport));
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
            first = FALSE;
            next = TRUE;
            cyg_httpd_write_chunked(";", 1);/* entry separator */
        }
        cyg_httpd_end_chunked();/* end of http responseText */
        T_DG(TRACE_GRP_WEB, "WEB - Refreshed entry table");
    }
    return -1;
}/* vcl group to vlan mapping configuration web handler */

static i32 handler_config_vcl_ip_based_conf(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                   sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    BOOL                          first = TRUE, next = FALSE, found_ports = FALSE;
    vcl_ip_mgmt_vce_conf_local_t  entry;
    vcl_ip_mgmt_vce_conf_global_t gentry;
    char                          ip_str[20];
    port_iter_t                   pit;
    int                           i, temp, vid, mask_port = 0;
    char                          search_str[32];
    mesa_port_list_t              ports;
    mesa_ipv4_t                   tmp_sub;
    i32                           vid_errors = 0;
    mesa_rc                       rc = VTSS_RC_OK;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }
#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VCL)) {
        return -1;
    }
#endif
    if (p->method == CYG_HTTPD_METHOD_POST) {/* POST Method */
        memset(search_str, 0, sizeof(search_str));
        /* Delete the entries with delete check button checked */
        T_DG(TRACE_GRP_WEB, "WEB - Deleting checked (old) entries");
        for (i = 1; i <= VCL_IP_VCE_MAX; i++) {
            sprintf(search_str, "delete_%d", i);
            if (cyg_httpd_form_varable_find(p, search_str)) {
                sprintf(search_str, "hiddenip_%d", i);
                if (cyg_httpd_form_varable_ipv4(p, search_str, &tmp_sub)) { /* Get IP address */
                    vtss_clear(entry);
                    entry.ip_addr = tmp_sub;
                    sprintf(search_str, "hiddenmask_%u", i);
                    if (!cyg_httpd_form_varable_int(p, search_str, &temp)) { /* Get Mask length */
                        continue; /* This should never happen */
                    }
                    entry.mask_len = temp;
                    if ((rc = vcl_ip_mgmt_conf_del(sid, &entry)) != VTSS_RC_OK) {
                        T_WG(TRACE_GRP_WEB, "WEB - %s", error_txt(rc));
                        if (rc == VCL_ERROR_ENTRY_NOT_FOUND) {
                            redirect(p, "/subnet_based_vlan.htm?IP_error=2");
                            return -1;
                        }
                    }
                }
            }
        }
        T_DG(TRACE_GRP_WEB, "WEB - Deleted checked (old) entries successfully");
        memset(search_str, 0, sizeof(search_str));
        /* Update the existing entries if ports are changed */
        T_DG(TRACE_GRP_WEB, "WEB - Editing updated (old) entries");
        for (i = 1; i <= VCL_IP_VCE_MAX; i++) {
            /* For deleted entries, don't check for updates */
            sprintf(search_str, "delete_%d", i);
            if (cyg_httpd_form_varable_find(p, search_str)) {
                continue;
            }
            sprintf(search_str, "hiddenip_%d", i);
            if (cyg_httpd_form_varable_ipv4(p, search_str, &tmp_sub)) {
                vtss_clear(entry);
                vtss_clear(gentry);
                memset(ports, 0, sizeof(ports));
                gentry.ip_addr = tmp_sub;
                if (!cyg_httpd_form_variable_int_fmt(p, &temp, "hiddenmask_%u", i)) { /* Get Mask length */
                    continue; /* This should never happen */
                }
                gentry.mask_len = temp;
                if (!cyg_httpd_form_variable_int_fmt(p, &vid, "hiddenvid_%u", i)) { /* Get VLAN ID */
                    continue; /* This should never happen */
                }
                T_NG(TRACE_GRP_WEB, "WEB - Editing old entry with subnet %s/%u and VID %u", misc_ipv4_txt(tmp_sub, ip_str), temp, vid);
                mask_port = 0;
                (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                while (port_iter_getnext(&pit)) {
                    mask_port++;
                    sprintf(search_str, "mask_%d_%d", i, mask_port);
                    if (cyg_httpd_form_varable_find(p, search_str)) { /* "on" if checked */
                        ports[pit.iport] = TRUE;
                        found_ports = TRUE;
                    }
                }
                if (found_ports == FALSE) {
                    redirect(p, "/subnet_based_vlan.htm?IP_error=3");
                    return -1;
                }
                if ((vcl_ip_mgmt_conf_get(sid, &gentry, FALSE, FALSE) == VTSS_RC_OK) && (gentry.ip_addr == tmp_sub)
                    && (gentry.mask_len == temp)) {
                    entry.ip_addr = gentry.ip_addr;
                    entry.mask_len = gentry.mask_len;
                    entry.vid = gentry.vid;
                    memcpy(entry.ports, gentry.ports[sid - VTSS_ISID_START], sizeof(entry.ports));
                    /* If VID and Ports are not changed, we need not update the entry */
                    if ((entry.vid != vid) || (memcmp(ports, entry.ports, sizeof(ports)))) {
                        entry.ip_addr = tmp_sub;
                        entry.mask_len = temp;
                        entry.vid = vid;
                        memcpy(entry.ports, ports, sizeof(ports));
                        if ((rc = vcl_ip_mgmt_conf_add(sid, &entry)) != VTSS_RC_OK) {
                            if (rc == VCL_ERROR_ENTRY_DIFF_VID) {
                                vid_errors++;
                            }
                            T_WG(TRACE_GRP_WEB, "WEB - %s", error_txt(rc));
                        }
                    }
                }
            }
        }
        if (vid_errors == 0) {
            T_DG(TRACE_GRP_WEB, "WEB - Edited updated (old) entries successfully");
        }
        /* Add new entries */
        T_DG(TRACE_GRP_WEB, "WEB - Adding new entries");
        for (i = 1; i <= VCL_IP_VCE_MAX; i++) {
            sprintf(search_str, "ipid_new_%d", i);
            if (cyg_httpd_form_varable_ipv4(p, search_str, &tmp_sub)) {
                vtss_clear(entry);
                entry.ip_addr = tmp_sub;
                if (!cyg_httpd_form_variable_int_fmt(p, &temp, "mask_bits_new_%u", i)) { /* Get Mask length */
                    continue; /* This should never happen */
                }
                entry.mask_len = temp;
                if (!cyg_httpd_form_variable_int_fmt(p, &vid, "vid_new_%u", i)) { /* Get VLAN ID */
                    continue; /* This should never happen */
                }
                entry.vid = vid;
                mask_port = 0;
                (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                while (port_iter_getnext(&pit)) {
                    mask_port++;
                    sprintf(search_str, "mask_new_%d_%d", i, mask_port);
                    if (cyg_httpd_form_varable_find(p, search_str)) { /* "on" if checked */
                        entry.ports[pit.iport] = TRUE;
                    } /* if(cyg_httpd_form_varable_find(p, search_str) */
                } /* while (port_iter_getnext(&pit)) */
                if ((rc = vcl_ip_mgmt_conf_add(sid, &entry)) != VTSS_RC_OK) {
                    if (rc == VCL_ERROR_ENTRY_DIFF_VID) {
                        vid_errors++;
                    }
                    if (rc == VCL_ERROR_INVALID_SUBNET) {
                        redirect(p, "/subnet_based_vlan.htm?IP_error=4");
                        return -1;
                    }
                    T_DG(TRACE_GRP_WEB, "WEB - %s", error_txt(rc));
                }
            }
        }
        if (vid_errors == 0) {
            T_DG(TRACE_GRP_WEB, "WEB - Added new entries successfully");
        }
        redirect(p, vid_errors ? "/subnet_based_vlan.htm?IP_error=1" : "/subnet_based_vlan.htm");
        return -1;
    } else { /* GET Method */
        i32 ct;
        T_DG(TRACE_GRP_WEB, "WEB - Refreshing entry table");
        cyg_httpd_start_chunked("html");
        while (vcl_ip_mgmt_conf_get(sid, &gentry, first, next) == VTSS_RC_OK) {
            entry.ip_addr = gentry.ip_addr;
            entry.mask_len = gentry.mask_len;
            entry.vid = gentry.vid;
            memcpy(entry.ports, gentry.ports[sid - 1], sizeof(entry.ports));
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s/%u/%u/",
                          misc_ipv4_txt(entry.ip_addr, ip_str), entry.mask_len, entry.vid);
            cyg_httpd_write_chunked(p->outbuffer, ct);
            /* Send member ports list for current entry */
            (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_ALL);
            while (port_iter_getnext(&pit)) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,", entry.ports.get(pit.iport));
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
            first = FALSE;
            next = TRUE;
            cyg_httpd_write_chunked(";", 1);
        }
        cyg_httpd_end_chunked();
        T_DG(TRACE_GRP_WEB, "WEB - Refreshed entry table");
    }

    return -1;
} /* handler_config_vcl_ip_based_conf */

/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/

static size_t vcl_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[VCL_WEB_BUF_LEN];
    (void) snprintf(buff, VCL_WEB_BUF_LEN,
                    "var configVCLMacIdMin = %d;\n"
                    "var configVCLMacIdMax = %d;\n"
                    "var configVCLIPIdMin = %d;\n"
                    "var configVCLIPIdMax = %d;\n"
                    "var configVCLProto2GrpMin = %d;\n"
                    "var configVCLProto2GrpMax = %d;\n"
                    "var configVCLGrp2VLANMin = %d;\n"
                    "var configVCLGrp2VLANMax = %d;\n",
                    1, /* First MAC-based VLAN ID */
                    VCL_MAC_VCE_MAX,   /* Last VCL MAC Entry (entry number, not absolute ID) */
                    1, /* First IP-based VLAN ID */
                    VCL_IP_VCE_MAX,    /* Last VCL IP Entry (entry number, not absolute ID) */
                    1, /* First protocol entry */
                    VCL_PROTO_PROTOCOL_MAX, /* Last VCL Protocol entry (entry number, not absolute ID) */
                    1, /* First protocol-based VLAN */
                    VCL_PROTO_VCE_MAX   /* Last VCL Protocol VLAN entry (entry number, not absolute ID) */
                   );
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  JS lib config table entry                                               */
/****************************************************************************/
web_lib_config_js_tab_entry(vcl_lib_config_js);

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

/****************************************************************************/
/*  Common JAGUAR/Luton26 table entries                                     */
/****************************************************************************/
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_vcl_mac_based_handler, "/config/vcl_conf", handler_config_vcl_mac_based_conf);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_vcl_proto_2_grp_map_handler, "/config/vcl_proto_2_grp_map", handler_config_vcl_proto2grp_map);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_vcl_grp_2_vlan_map_handler, "/config/vcl_grp_2_vlan_map", handler_config_vcl_grp2vlan_map);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_vcl_ip_based_handler, "/config/vcl_ip_conf", handler_config_vcl_ip_based_conf);

