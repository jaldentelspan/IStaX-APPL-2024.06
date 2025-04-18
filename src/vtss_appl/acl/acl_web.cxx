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
#include "misc_api.h"
#include "msg_api.h"   // msg_switch_exists(), msg_switch_configurable()

#ifdef VTSS_SW_OPTION_ACL
#include "acl_api.h"
#include "mgmt_api.h"
#endif /* VTSS_SW_OPTION_ACL */
#include "port_api.h" // For port_count_max()
#include "msg_api.h"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

#define ACL_WEB_BUF_LEN 1024

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB
#include <vtss_trace_api.h>
/* ============== */

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/

static void ace_overview(CYG_HTTPD_STATE *p, acl_user_t user_id, mesa_ace_id_t id, acl_entry_conf_t *ace_conf, mesa_ace_counter_t ace_counter, BOOL local_status)
{
    int     ct;
    char    str_buff1[24], str_buff2[24], str_buff3[24], str_buff4[24], str_buff5[24];
    int     i, any = 1;
    BOOL    udp = 0, tcp = 0;
    int     var_value1 = 0, var_value2 = 0, var_value3 = 0;
    u32     sip_v6_mask;
    char    buf[MGMT_PORT_BUF_SIZE];
    char    encoded_string[3 * 80];
    int     port_list_cnt, port_filter_list_cnt;
    BOOL    is_filter_enable;
    u32     port_count = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    //dmac_filter
    var_value2 = VTSS_BF_GET(ace_conf->flags.mask, ACE_FLAG_DMAC_BC) ? (VTSS_BF_GET(ace_conf->flags.value, ACE_FLAG_DMAC_BC) ? 1 : 0) : 2;
    var_value3 = VTSS_BF_GET(ace_conf->flags.mask, ACE_FLAG_DMAC_MC) ? (VTSS_BF_GET(ace_conf->flags.value, ACE_FLAG_DMAC_MC) ? 1 : 0) : 2;
    if (var_value2 == 2 && var_value3 == 2) {
        var_value1 = 0; //any
    } else if (var_value2 == 0 && var_value3 == 1) {
        var_value1 = 1; //MC
    } else if (var_value2 == 1 && var_value3 == 1) {
        var_value1 = 2; //BC
    } else if (var_value2 == 0 && var_value3 == 0) {
        var_value1 = 3; //UC
    }

    //dmac
    any = 1;
    for (i = 0; i < 6; i++) {
        if (ace_conf->frame.etype.dmac.mask[i]) {
            any = 0;
            break;
        }
    }
    if (any || (ace_conf->type != MESA_ACE_TYPE_ETYPE)) {
        strcpy(str_buff1, "00-00-00-00-00-02");
    } else {
        var_value1 = 4; //Specific
        (void) misc_mac_txt(ace_conf->frame.etype.dmac.value, str_buff1);
    }

    if (local_status) {
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/", user_id);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/", ace_conf->action.force_cpu);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/", ace_conf->action.cpu_once);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/", ace_conf->conflict);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
    }

    for (port_list_cnt = 0, port_filter_list_cnt = 0, i = 0; i < port_count; i++) {
        if (ace_conf->port_list[i]) {
            port_list_cnt++;
        }
        if (ace_conf->action.port_list[i]) {
            port_filter_list_cnt++;
        }
    }
    if (ace_conf->action.port_action == MESA_ACL_PORT_ACTION_FILTER && port_filter_list_cnt != 0) {
        is_filter_enable = TRUE;
    } else {
        is_filter_enable = FALSE;
    }

    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%u/%u/%u",
                  VTSS_ISID_LOCAL,
                  ace_conf->id,
                  id,
                  ace_conf->action.port_action == MESA_ACL_PORT_ACTION_NONE ? 1 : is_filter_enable ? 2 : 0);
    (void)cyg_httpd_write_chunked(p->outbuffer, ct);

    if (port_list_cnt == port_count) {
        strcpy(encoded_string, "All");
    } else {
        if (cgi_escape(mgmt_iport_list2txt(ace_conf->port_list, buf), encoded_string) == 0) {
            strcpy(encoded_string, "Disabled");
        }
    }
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s",  encoded_string);
    (void)cyg_httpd_write_chunked(p->outbuffer, ct);

    if (!local_status) {
        // policy_filter, policy, policy_bitmask
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/%u/%X",
                      ace_conf->policy.mask == 0x0 ? 0 : 1,
                      ace_conf->policy.value,
                      ace_conf->policy.mask);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
    }

    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u", ace_conf->action.policer == ACL_MGMT_RATE_LIMITER_NONE ? 0 : ipolicer2upolicer(ace_conf->action.policer));
    (void)cyg_httpd_write_chunked(p->outbuffer, ct);

    if (ace_conf->action.port_action == MESA_ACL_PORT_ACTION_REDIR) {
        if (cgi_escape(mgmt_iport_list2txt(ace_conf->action.port_list, buf), encoded_string) == 0) {
            strcpy(encoded_string, "Disabled");
        }
    } else {
        strcpy(encoded_string, "Disabled");
    }
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s/%u",
                  encoded_string,
                  ace_conf->action.mirror);
    (void)cyg_httpd_write_chunked(p->outbuffer, ct);

    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/%s/%u/%u",
                  var_value1,
                  str_buff1,
                  ace_counter,
                  ace_conf->type);
    (void)cyg_httpd_write_chunked(p->outbuffer, ct);

    if (ace_conf->type == MESA_ACE_TYPE_ETYPE) {

        any = 1;
        for (i = 0; i < 6; i++) {
            if (ace_conf->frame.etype.smac.mask[i]) {
                any = 0;
                break;
            }
        }
        if (any) {
            strcpy(str_buff1, "/0/00-00-00-00-00-01");
        } else {
            strcpy(str_buff1, "/1/");
            (void) misc_mac_txt(ace_conf->frame.etype.smac.value, &str_buff1[4]);
        }

        if (ace_conf->frame.etype.etype.mask[0] || ace_conf->frame.etype.etype.mask[1]) {
            sprintf(str_buff2, "1/%02x%02x", ace_conf->frame.etype.etype.value[0], ace_conf->frame.etype.etype.value[1]);
        } else {
            sprintf(str_buff2, "0/0");
        }

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s/%s", str_buff1, str_buff2);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
    } else if (ace_conf->type == MESA_ACE_TYPE_ARP) {

        any = 1;
        for (i = 0; i < 6; i++) {
            if (ace_conf->frame.arp.smac.mask[i]) {
                any = 0;
                break;
            }
        }
        if (any) {
            strcpy(str_buff1, "/0/00-00-00-00-00-01");
        } else {
            strcpy(str_buff1, "/1/");
            (void) misc_mac_txt(ace_conf->frame.arp.smac.value, &str_buff1[4]);
        }

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s/%u/%s/%s/%u/%s/%s",
                      str_buff1,
                      ace_conf->frame.arp.sip.mask != 0 ? (ace_conf->frame.arp.sip.mask == 0xFFFFFFFF ? 1 : 2 ) : 0,
                      misc_ipv4_txt(ace_conf->frame.arp.sip.value, str_buff2),
                      misc_ipv4_txt(ace_conf->frame.arp.sip.mask, str_buff3),
                      ace_conf->frame.arp.dip.mask != 0 ? (ace_conf->frame.arp.dip.mask == 0xFFFFFFFF ? 1 : 2 ) : 0,
                      misc_ipv4_txt(ace_conf->frame.arp.dip.value, str_buff4),
                      misc_ipv4_txt(ace_conf->frame.arp.dip.mask, str_buff5));
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
    } else if (ace_conf->type == MESA_ACE_TYPE_IPV4) {

        udp = (ace_conf->frame.ipv4.proto.value == 17);
        tcp = (ace_conf->frame.ipv4.proto.value == 6);

        if (ace_conf->frame.ipv4.proto.mask) {
            if (ace_conf->frame.ipv4.proto.value == 1) { //ICMP
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/1/%u", ace_conf->frame.ipv4.proto.value);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            } else if (udp) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/2/%u", ace_conf->frame.ipv4.proto.value);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            } else if (tcp) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/3/%u", ace_conf->frame.ipv4.proto.value);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            } else {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/4/%u", ace_conf->frame.ipv4.proto.value);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        } else { //Any
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/0/%u", ace_conf->frame.ipv4.proto.value);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        //sip, dip
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/%u/%s/%s/%u/%s/%s",
                      VTSS_BF_GET(ace_conf->flags.mask, ACE_FLAG_IP_FRAGMENT) ? (VTSS_BF_GET(ace_conf->flags.value, ACE_FLAG_IP_FRAGMENT) ? 1 : 0) : 2,
                      ace_conf->frame.ipv4.sip_smac.enable ? 1 : (ace_conf->frame.ipv4.sip.mask != 0 ? (ace_conf->frame.ipv4.sip.mask == 0xFFFFFFFF ? 1 : 2 ) : 0),
                      ace_conf->frame.ipv4.sip_smac.enable ? misc_ipv4_txt(ace_conf->frame.ipv4.sip_smac.sip, str_buff1) : misc_ipv4_txt(ace_conf->frame.ipv4.sip.value, str_buff1),
                      ace_conf->frame.ipv4.sip_smac.enable ? "255.255.255.255" : misc_ipv4_txt(ace_conf->frame.ipv4.sip.mask, str_buff2),
                      ace_conf->frame.ipv4.dip.mask != 0 ? (ace_conf->frame.ipv4.dip.mask == 0xFFFFFFFF ? 1 : 2) : 0,
                      misc_ipv4_txt(ace_conf->frame.ipv4.dip.value, str_buff3),
                      misc_ipv4_txt(ace_conf->frame.ipv4.dip.mask, str_buff4));
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        if (ace_conf->frame.ipv4.proto.value == 1) {
            //icmp_type, icmp_code
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/%u/%u/%u",
                          ace_conf->frame.ipv4.data.mask[0] == 0 ? 0 : 1,
                          ace_conf->frame.ipv4.data.value[0],
                          ace_conf->frame.ipv4.data.mask[1] == 0 ? 0 : 1,
                          ace_conf->frame.ipv4.data.value[1]);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        //sport_range, dport_range
        if (udp || tcp) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/%d/%d/%u/%d/%d",
                          (ace_conf->frame.ipv4.sport.low == 0 && ace_conf->frame.ipv4.sport.high == 65535) ? 0 : ace_conf->frame.ipv4.sport.low == ace_conf->frame.ipv4.sport.high ? 1 : 2,
                          ace_conf->frame.ipv4.sport.low,
                          ace_conf->frame.ipv4.sport.high,
                          (ace_conf->frame.ipv4.dport.low == 0 && ace_conf->frame.ipv4.dport.high == 65535) ? 0 : ace_conf->frame.ipv4.dport.low == ace_conf->frame.ipv4.dport.high ? 1 : 2,
                          ace_conf->frame.ipv4.dport.low,
                          ace_conf->frame.ipv4.dport.high);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        //tcp_flags
        if (tcp) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/%u/%u/%u/%u/%u",
                          VTSS_BF_GET(ace_conf->flags.mask, ACE_FLAG_TCP_FIN) ? (VTSS_BF_GET(ace_conf->flags.value, ACE_FLAG_TCP_FIN) ? 1 : 0) : 2,
                          VTSS_BF_GET(ace_conf->flags.mask, ACE_FLAG_TCP_SYN) ? (VTSS_BF_GET(ace_conf->flags.value, ACE_FLAG_TCP_SYN) ? 1 : 0) : 2,
                          VTSS_BF_GET(ace_conf->flags.mask, ACE_FLAG_TCP_RST) ? (VTSS_BF_GET(ace_conf->flags.value, ACE_FLAG_TCP_RST) ? 1 : 0) : 2,
                          VTSS_BF_GET(ace_conf->flags.mask, ACE_FLAG_TCP_PSH) ? (VTSS_BF_GET(ace_conf->flags.value, ACE_FLAG_TCP_PSH) ? 1 : 0) : 2,
                          VTSS_BF_GET(ace_conf->flags.mask, ACE_FLAG_TCP_ACK) ? (VTSS_BF_GET(ace_conf->flags.value, ACE_FLAG_TCP_ACK) ? 1 : 0) : 2,
                          VTSS_BF_GET(ace_conf->flags.mask, ACE_FLAG_TCP_URG) ? (VTSS_BF_GET(ace_conf->flags.value, ACE_FLAG_TCP_URG) ? 1 : 0) : 2);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }
    } else if (ace_conf->type == MESA_ACE_TYPE_IPV6) {
        int sipv6_filter = 0;
        char ipv6_str_buf[40];
        mesa_ipv6_t ipv6_addr;

        memset(&ipv6_addr, 0, sizeof(ipv6_addr));
        for (i = 0; i < 16; i++) {
            if (ace_conf->frame.ipv6.sip.mask[i]) {
                sipv6_filter = 1;
                memcpy(&ipv6_addr, ace_conf->frame.ipv6.sip.value, sizeof(ipv6_addr));
                break;
            }
        }
        sip_v6_mask = (ace_conf->frame.ipv6.sip.mask[12] << 24) |
                      (ace_conf->frame.ipv6.sip.mask[13] << 16) |
                      (ace_conf->frame.ipv6.sip.mask[14] << 8)  |
                      ace_conf->frame.ipv6.sip.mask[15];
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d/%d/%d/%s/%X",
                      ace_conf->frame.ipv6.proto.mask ? 1 : 0,
                      ace_conf->frame.ipv6.proto.mask ? ace_conf->frame.ipv6.proto.value : 0,
                      sipv6_filter,
                      sipv6_filter ? misc_ipv6_txt(&ipv6_addr, ipv6_str_buf) : " ",
                      sip_v6_mask);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
    }

    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s", "|");
    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
}

static i32 handler_config_acl(CYG_HTTPD_STATE *p)
{
    int                 ct;
    mesa_rc             rc;
    mesa_ace_id_t       ace_id = ACL_MGMT_ACE_ID_NONE;
    acl_entry_conf_t    ace_conf, newconf;
    mesa_ace_counter_t  ace_counter;
    int                 ace_flag = 0, var_value = 0;
    const char          *var_string;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_GET) { /* CYG_HTTPD_METHOD_GET (+HEAD) */
        /* Format: [flag]/[date]
           <Del>   : 1/[ace_id]
           <Move>  : 2/[next_id]
           <clear> : 3/0
        */
        if ((var_string = cyg_httpd_form_varable_find(p, "aceConfigFlag")) != NULL) {
            ace_flag = atoi(var_string);
            if ((var_string = cyg_httpd_form_varable_find(p, "SelectAceId")) != NULL) {
                var_value = atoi(var_string);
            }
            switch (ace_flag) {
            case 1:
                (void) acl_mgmt_ace_del(ACL_USER_STATIC, var_value);
                break;
            case 2:
                if (acl_mgmt_ace_get(ACL_USER_STATIC, var_value, &ace_conf, NULL, 1) == VTSS_RC_OK) {
                    (void)acl_mgmt_ace_add(ACL_USER_STATIC, var_value, &ace_conf);
                }
                break;
            case 3:
                if (acl_mgmt_counters_clear() != VTSS_RC_OK) {
                    T_W("acl_mgmt_counters_clear() failed");
                }
                break;
            case 4:
                while (acl_mgmt_ace_get(ACL_USER_STATIC, ace_id, &ace_conf, NULL, 1) == VTSS_RC_OK) {
                    if (acl_mgmt_ace_del(ACL_USER_STATIC, ace_conf.id) != VTSS_RC_OK) {
                        T_W("acl_mgmt_ace_del() failed");
                    }
                }
                break;
            default:
                break;
            }
        }

        /* get form data
           Format: [isid]/[ace_id]/[next_ace_id]/[action]/[ingress_port]/[policy_filter]/[policy]/[policy_bitmask]/[rate_limiter]/[port_copy_1]:[port_copy_2]:.../[mirror]/[dmac_filter]/[dmac_mac]/[counters]/[frame_type]/[frame_type_filed]|...,[first_bind_ace_id]
           [frame_type_filed]
           - [frame_type] = Any
           - [frame_type] = Ethernet [smac_filter]/[smac]/[ether_type_filter]/[ether_type]
           - [frame_type] = ARP      [smac_filter]/[smac]/[arp_sip_filter]/[arp_sip]/[arp_sip_mask]/[arp_dip_filter]/[arp_dip]/[arp_dip_mask]
           - [frame_type] = IPv4, [protocol] = Any/Other [protocol_filter]/[protocol]/[ip_flags1]/[sip_filter]/[sip]/[sip_mask]/[dip_filter]/[dip]/[dip_mask]
           - [frame_type] = IPv4, [protocol] = ICMP      [protocol_filter]/[protocol]/[ip_flags1]/[sip_filter]/[sip]/[sip_mask]/[dip_filter]/[dip]/[dip_mask]/[icmp_type_filter]/[icmp_type]/[icmp_code_filter]/[icmp_code]
           - [frame_type] = IPv4, [protocol] = UDP       [protocol_filter]/[protocol]/[ip_flags1]/[sip_filter]/[sip]/[sip_mask]/[dip_filter]/[dip]/[dip_mask]/[sport_filter]/[sport_low]/[sport_high]/[dport_filter]/[dport_low]/[dport_high]
           - [frame_type] = IPv4, [protocol] = TCP       [protocol_filter]/[protocol]/[ip_flags1]/[sip_filter]/[sip]/[sip_mask]/[dip_filter]/[dip]/[dip_mask]/[sport_filter]/[sport_low]/[sport_high]/[dport_filter]/[dport_low]/[dport_high]/[tcp_flags0]/[tcp_flags1]/[tcp_flags2]/[tcp_flags3]/[tcp_flags4]/[tcp_flags5]
           - [frame_type] = IPv6  [next_header_filter]/[next_header]/[sip_v6_filter]/[sip_v6]/[sip_v6_mask]
        */
        (void)cyg_httpd_start_chunked("html");

        /* Show user defined ACEs */
        ace_id = 0;
        while (acl_mgmt_ace_get(ACL_USER_STATIC, ace_id, &ace_conf, &ace_counter, 1) == VTSS_RC_OK) {
            rc = acl_mgmt_ace_get(ACL_USER_STATIC, ace_conf.id, &newconf, NULL, 1);
            ace_overview(p, ACL_USER_STATIC, rc == VTSS_RC_OK ? newconf.id : ACL_MGMT_ACE_ID_NONE, &ace_conf, ace_counter, 0);
            ace_id = ace_conf.id;
        }
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ",65535");
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static int acl_tag_prio_range_mapping(mesa_ace_u8_t usr_prio)
{
    int value;

    if (usr_prio.mask == 0) {
        value = 14;
    } else if (usr_prio.mask == 0x7) {
        return usr_prio.value;
    } else if (usr_prio.mask == 6) {
        if (usr_prio.value == 0) { //0-1
            value = 8;
        } else if (usr_prio.value == 2) { //2-3
            value = 9;
        } else if (usr_prio.value == 4) { //4-5
            value = 10;
        } else { //6-7
            value = 11;
        }
    } else {
        if (usr_prio.value == 0) { //0-3
            value = 12;
        } else { //4-7
            value = 13;
        }
    }

    return value;
}

static i32 handler_config_acl_edit(CYG_HTTPD_STATE *p)
{
    int                 ct;
    mesa_rc             rc;
    mesa_ace_id_t       ace_id = ACL_MGMT_ACE_ID_NONE, next_ace_id = ACL_MGMT_ACE_ID_NONE;
    acl_entry_conf_t    ace_conf, newconf;
    mesa_ace_counter_t  ace_counter;
    char                str_buff1[24], str_buff2[24], str_buff3[24], str_buff4[24], str_buff5[24];
    int                 i, j, any = 1;
    BOOL                icmp = 0, udp = 0, tcp = 0;
    char                var_name1[32];
    int                 ace_flag = 0;
    int                 var_action, var_value1 = 0, var_value2 = 0, var_value3 = 0, var_value4 = 0;
    ulong               var_ulong_value3 = 0;

    const char          *var_string;
    u32                 sip_v6_mask;
    char                new_url[128];
    u32                 ace_config_flag = 3, select_ace_Id = 0;
    int                 is_first, port_list_cnt, port_filter_list_cnt;
    BOOL                is_filter_enable;
    char                buf[80];
    u32                 port_count = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;
        /* store form data */
        (void) acl_mgmt_ace_init(MESA_ACE_TYPE_ANY, &newconf);

        //switch_id

        //ace_id, next_ace_id
        if (cyg_httpd_form_varable_int(p, "ace_id", &var_value1)) {
            newconf.id = var_value1;
        }
        if (cyg_httpd_form_varable_int(p, "next_ace_id", &var_value1)) {
            next_ace_id = var_value1;
        }

        // Format        : [aceConfigFlag]/[SelectAceId]
        // <Edit>        :               1/[ace_id]
        // <Insert>      :               2/[next_id]
        // <Add to Last> :               3/0
        if (newconf.id != ACL_MGMT_ACE_ID_NONE) {
            ace_config_flag = 1;
            select_ace_Id = newconf.id;
        } else if (next_ace_id == ACL_MGMT_ACE_ID_NONE) {
            ace_config_flag = 3;
            select_ace_Id = 0;
        } else {
            ace_config_flag = 2;
            select_ace_Id = next_ace_id;
        }
        sprintf(new_url, "acl_edit.htm?aceConfigFlag=%d&SelectAceId=%d", ace_config_flag, select_ace_Id);

        //lookup
        if (fast_cap(MESA_CAP_ACL_KEY_LOOKUP) && cyg_httpd_form_varable_int(p, "lookup", &var_value1)) {
            newconf.lookup = var_value1;
        }

        //action
        if (cyg_httpd_form_varable_int(p, "action", &var_action)) {
            if (var_action != 2) { //permit|deny
                newconf.action.port_action = (var_action ? MESA_ACL_PORT_ACTION_NONE : MESA_ACL_PORT_ACTION_FILTER);
                for (i = 0; i < port_count; i++) {
                    newconf.action.port_list[i] = var_action;
                }
            }
        }

        //filter_port
        if (var_action == 2) { //filter
            newconf.action.port_action = MESA_ACL_PORT_ACTION_FILTER;
            var_value2 = 0;
            i = 1;
            memset(newconf.action.port_list, 0, sizeof(newconf.action.port_list));
            while (cyg_httpd_form_multi_varable_int(p, "filter_port", &var_value2, i++)) {
                if (var_value2 == 0) { // All port
                    for (i = 0; i < port_count; i++) {
                        newconf.action.port_list[i] = TRUE;
                    }
                    break;
                }
                newconf.action.port_list[uport2iport(var_value2)] = TRUE;
            }
        }

        //ingress_port
        var_value2 = 0;
        i = 1;
        memset(newconf.port_list, 0, sizeof(newconf.port_list));
        while (cyg_httpd_form_multi_varable_int(p, "ingress_port", &var_value2, i++)) {
            if (var_value2 == 0) { // All port
                for (i = 0; i < port_count; i++) {
                    newconf.port_list[i] = TRUE;
                }
                break;
            }
            newconf.port_list[uport2iport(var_value2)] = TRUE;
        }

        //policy_filter, policy, policy_bitmask
        if (cyg_httpd_form_varable_int(p, "policy_filter", &var_value1)) {
            if (var_value1 != 0) {
                if (cyg_httpd_form_varable_int(p, "policy", &var_value2) &&
                    cyg_httpd_form_varable_hex(p, "policy_bitmask", &var_ulong_value3)) {
                    newconf.policy.value = var_value2;
                    newconf.policy.mask = var_ulong_value3 < ACL_MGMT_POLICIES_BITMASK ? var_ulong_value3 : ACL_MGMT_POLICIES_BITMASK;
                }
            } else {
                newconf.policy.value = newconf.policy.mask = 0;
            }
        }

        //rate_limiter
        if (cyg_httpd_form_varable_int(p, "rate_limiter", &var_value1)) {
            newconf.action.policer = (var_value1 == 0 ? ACL_MGMT_RATE_LIMITER_NONE : upolicer2ipolicer(var_value1));
        }

        //port_copy
        if (var_action == 0) {
            i = 1;
            while (cyg_httpd_form_multi_varable_int(p, "port_copy", &var_value1, i++)) {
                if (var_value1 == 0) { /* Disabled */
                    break;
                } else {
                    if (i == 1) {
                        memset(newconf.action.port_list, 0, sizeof(newconf.action.port_list));
                    }
                    newconf.action.port_action = MESA_ACL_PORT_ACTION_REDIR;
                    newconf.action.port_list[uport2iport(var_value1)] = TRUE;
                }
            }
        }

        //mirror
        if (cyg_httpd_form_varable_int(p, "mirror", &var_value1)) {
            newconf.action.mirror = var_value1;
        }

        //logging, shutdown
        if (cyg_httpd_form_varable_int(p, "logging", &var_value1)) {
            newconf.action.logging = var_value1;
        }
        if (cyg_httpd_form_varable_int(p, "shutdown", &var_value1)) {
            newconf.action.shutdown = var_value1;
        }

        //tagged
        if (cyg_httpd_form_varable_int(p, "tagged", &var_value1)) {
            newconf.tagged = (mesa_ace_bit_t)var_value1;
        }

        //vid_filter, vid
        if (cyg_httpd_form_varable_int(p, "vid_filter", &var_value1)) {
            if (cyg_httpd_form_varable_int(p, "vid", &var_value2)) {
                if (var_value1 == 1) {
                    newconf.vid.mask = 0xfff;
                    newconf.vid.value = var_value2;
                }
            }
        }

        //tag_priority
        if (cyg_httpd_form_varable_int(p, "tag_priority", &var_value1)) {
            if (var_value1 <= 7) {
                newconf.usr_prio.mask = 0x7;
                newconf.usr_prio.value = var_value1;
            } else if (var_value1 == 8) { //0-1
                newconf.usr_prio.mask = 0x6;
                newconf.usr_prio.value = 0;
            } else if (var_value1 == 9) { //2-3
                newconf.usr_prio.mask = 0x6;
                newconf.usr_prio.value = 2;
            } else if (var_value1 == 10) { //4-5
                newconf.usr_prio.mask = 0x6;
                newconf.usr_prio.value = 4;
            } else if (var_value1 == 11) { //6-7
                newconf.usr_prio.mask = 0x6;
                newconf.usr_prio.value = 6;
            } else if (var_value1 == 12) { //0-3
                newconf.usr_prio.mask = 0x4;
                newconf.usr_prio.value = 0;
            } else if (var_value1 == 13) { //4-7
                newconf.usr_prio.mask = 0x4;
                newconf.usr_prio.value = 4;
            } else {
                newconf.usr_prio.mask = 0x0;
                newconf.usr_prio.value = 0;
            }
        }

        //cfi
        if (cyg_httpd_form_varable_int(p, "cfi", &var_value1)) {
            if (var_value1 <= 1) {
                VTSS_BF_SET(newconf.flags.mask, ACE_FLAG_VLAN_CFI, 1);
                VTSS_BF_SET(newconf.flags.value, ACE_FLAG_VLAN_CFI, var_value1);
            }
        }

        //dmac
        if (cyg_httpd_form_varable_int(p, "dmac_filter", &var_value2)) {
            if (var_value2 == 1) { //MC
                VTSS_BF_SET(newconf.flags.mask, ACE_FLAG_DMAC_MC, 1);
                VTSS_BF_SET(newconf.flags.value, ACE_FLAG_DMAC_MC, 1);
                VTSS_BF_SET(newconf.flags.mask, ACE_FLAG_DMAC_BC, 1);
                VTSS_BF_SET(newconf.flags.value, ACE_FLAG_DMAC_BC, 0);
            } else if (var_value2 == 2) { //BC
                VTSS_BF_SET(newconf.flags.mask, ACE_FLAG_DMAC_MC, 1);
                VTSS_BF_SET(newconf.flags.value, ACE_FLAG_DMAC_MC, 1);
                VTSS_BF_SET(newconf.flags.mask, ACE_FLAG_DMAC_BC, 1);
                VTSS_BF_SET(newconf.flags.value, ACE_FLAG_DMAC_BC, 1);
            } else if (var_value2 == 3) { //UC
                VTSS_BF_SET(newconf.flags.mask, ACE_FLAG_DMAC_MC, 1);
                VTSS_BF_SET(newconf.flags.value, ACE_FLAG_DMAC_MC, 0);
                VTSS_BF_SET(newconf.flags.mask, ACE_FLAG_DMAC_BC, 1);
                VTSS_BF_SET(newconf.flags.value, ACE_FLAG_DMAC_BC, 0);
            } else if (var_value2 == 4) { //specific
                if (cyg_httpd_form_varable_mac(p, "dmac", newconf.frame.etype.dmac.value)) {
                    for (i = 0; i < 6; i++) {
                        newconf.frame.etype.dmac.mask[i] = 0xff;
                    }
                }
            }
        }

        //frame type
        if (cyg_httpd_form_varable_int(p, "frame_type", &var_value1)) {
            switch (var_value1) {
            case 0:
                newconf.type = MESA_ACE_TYPE_ANY;
                break;

            case 1:
                newconf.type = MESA_ACE_TYPE_ETYPE;

                //ether_type
                if (cyg_httpd_form_varable_int(p, "ether_type_filter", &var_value2)) {
                    if (var_value2 == 1) {
                        if (cyg_httpd_form_varable_hex(p, "ether_type", &var_ulong_value3)) {
                            newconf.frame.etype.etype.mask[0] = 0xff;
                            newconf.frame.etype.etype.mask[1] = 0xff;
                            newconf.frame.etype.etype.value[0] = var_ulong_value3 >> 8;
                            newconf.frame.etype.etype.value[1] = var_ulong_value3 & 0xff;
                        }
                    }
                }

                //smac
                if (cyg_httpd_form_varable_int(p, "smac_filter", &var_value2)) {
                    if (var_value2 == 1) {
                        if (cyg_httpd_form_varable_mac(p, "smac", newconf.frame.etype.smac.value)) {
                            for (i = 0; i < 6; i++) {
                                newconf.frame.etype.smac.mask[i] = 0xff;
                            }
                        }
                    }
                }
                break;

            case 2:
                newconf.type = MESA_ACE_TYPE_ARP;

                //smac
                if (cyg_httpd_form_varable_int(p, "smac_filter", &var_value2)) {
                    if (var_value2 == 1) {
                        if (cyg_httpd_form_varable_mac(p, "smac", newconf.frame.arp.smac.value)) {
                            for (i = 0; i < 6; i++) {
                                newconf.frame.arp.smac.mask[i] = 0xff;
                            }
                        }
                    }
                }

                //arp_sip, arp_dip
                if (cyg_httpd_form_varable_int(p, "arp_sip_filter", &var_value2)) {
                    if (var_value2 == 1 || var_value2 == 2) {
                        (void) cyg_httpd_form_varable_ipv4(p, "arp_sip", &newconf.frame.arp.sip.value);
                        newconf.frame.arp.sip.mask = 0xFFFFFFFF;
                    }
                    if (var_value2 == 2) { //network
                        (void) cyg_httpd_form_varable_ipv4(p, "arp_sip_mask", &newconf.frame.arp.sip.mask);
                    }
                }
                if (cyg_httpd_form_varable_int(p, "arp_dip_filter", &var_value2)) {
                    if (var_value2 == 1 || var_value2 == 2) {
                        (void) cyg_httpd_form_varable_ipv4(p, "arp_dip", &newconf.frame.arp.dip.value);
                        newconf.frame.arp.dip.mask = 0xFFFFFFFF;
                    }
                    if (var_value2 == 2) { //network
                        (void) cyg_httpd_form_varable_ipv4(p, "arp_dip_mask", &newconf.frame.arp.dip.mask);
                    }
                }

                //flag
                for (i = ACE_FLAG_ARP_ARP, j = 0; i < ACE_FLAG_IP_TTL; i++, j++) {
                    sprintf(var_name1, "flag%d", j);
                    if (cyg_httpd_form_varable_int(p, var_name1, &var_value2)) {
                        if (var_value2 <= 1) {
                            VTSS_BF_SET(newconf.flags.mask, i, 1);
                            VTSS_BF_SET(newconf.flags.value, i, var_value2);
                        }
                    }
                }

                if (cyg_httpd_form_varable_int(p, "arp_flags0", &var_value2)) {
                    if (var_value2 == 0) { //rarp
                        VTSS_BF_SET(newconf.flags.mask, ACE_FLAG_ARP_ARP, 1);
                        VTSS_BF_SET(newconf.flags.value, ACE_FLAG_ARP_ARP, 0);
                        VTSS_BF_SET(newconf.flags.mask, ACE_FLAG_ARP_UNKNOWN, 1);
                        VTSS_BF_SET(newconf.flags.value, ACE_FLAG_ARP_UNKNOWN, 0);
                    } else if (var_value2 == 1) { //arp
                        VTSS_BF_SET(newconf.flags.mask, ACE_FLAG_ARP_ARP, 1);
                        VTSS_BF_SET(newconf.flags.value, ACE_FLAG_ARP_ARP, 1);
                        VTSS_BF_SET(newconf.flags.mask, ACE_FLAG_ARP_UNKNOWN, 1);
                        VTSS_BF_SET(newconf.flags.value, ACE_FLAG_ARP_UNKNOWN, 0);
                    } else if (var_value2 == 3) { //other
                        VTSS_BF_SET(newconf.flags.mask, ACE_FLAG_ARP_UNKNOWN, 1);
                        VTSS_BF_SET(newconf.flags.value, ACE_FLAG_ARP_UNKNOWN, 1);
                    }
                }
                break;

            case 4:
                newconf.type = MESA_ACE_TYPE_IPV6;

                //next_header
                if (cyg_httpd_form_varable_int(p, "next_header_filter", &var_value2)) {
                    if (var_value2 == 4) {
                        if (cyg_httpd_form_varable_int(p, "next_header", &var_value3)) {
                            newconf.frame.ipv6.proto.mask = 0xff;
                            newconf.frame.ipv6.proto.value = var_value3 & 0xff;
                        }
                    }
                }

                //sip_v6
                if (cyg_httpd_form_varable_int(p, "sip_v6_filter", &var_value3)) {
                    if (var_value3 == 1) {
                        mesa_ipv6_t ipv6_addr;
                        (void)cyg_httpd_form_varable_ipv6(p, "sip_v6", &ipv6_addr);
                        for (i = 0; i < 16; i++) {
                            newconf.frame.ipv6.sip.value[i] = ipv6_addr.addr[i];
                            newconf.frame.ipv6.sip.mask[i] = 0x0;
                        }

                        if (cyg_httpd_form_varable_hex(p, "sip_v6_mask", &var_ulong_value3)) {
                            newconf.frame.ipv6.sip.mask[12] = (var_ulong_value3 & 0xff000000) >> 24;
                            newconf.frame.ipv6.sip.mask[13] = (var_ulong_value3 & 0x00ff0000) >> 16;
                            newconf.frame.ipv6.sip.mask[14] = (var_ulong_value3 & 0x0000ff00) >>  8;
                            newconf.frame.ipv6.sip.mask[15] = (var_ulong_value3 & 0x000000ff) >>  0;
                        }
                    }
                }

                if (cyg_httpd_form_varable_int(p, "hop_limit", &var_value3)) {
                    newconf.frame.ipv6.ttl = (var_value3 == 2 ? MESA_ACE_BIT_ANY : var_value3 == 0 ? MESA_ACE_BIT_0 : MESA_ACE_BIT_1);
                }

                if (var_value2 == 1) { //ICMP
                    newconf.frame.ipv6.proto.mask = 0xff;
                    newconf.frame.ipv6.proto.value = 58;

                    //icmp_type
                    if (cyg_httpd_form_varable_int(p, "icmp_type_filter", &var_value3)) {
                        if (var_value3 == 1) {
                            if (cyg_httpd_form_varable_int(p, "icmp_type", &var_value4)) {
                                newconf.frame.ipv6.data.mask[0] = 0xff;
                                newconf.frame.ipv6.data.value[0] = var_value4;
                            }
                        }
                    }

                    //icmp_code
                    if (cyg_httpd_form_varable_int(p, "icmp_code_filter", &var_value3)) {
                        if (var_value3 == 1) {
                            if (cyg_httpd_form_varable_int(p, "icmp_code", &var_value4)) {
                                newconf.frame.ipv6.data.mask[1] = 0xff;
                                newconf.frame.ipv6.data.value[1] = var_value4;
                            }
                        }
                    }
                } else if (var_value2 == 2) { //UDP
                    newconf.frame.ipv6.proto.mask = 0xff;
                    newconf.frame.ipv6.proto.value = 17;
                } else if (var_value2 == 3) { //TCP
                    newconf.frame.ipv6.proto.mask = 0xff;
                    newconf.frame.ipv6.proto.value = 6;
                }

                //sport, dport range
                newconf.frame.ipv6.sport.in_range = newconf.frame.ipv6.dport.in_range = 1;
                newconf.frame.ipv6.sport.low = newconf.frame.ipv6.dport.low = 0;
                newconf.frame.ipv6.sport.high = newconf.frame.ipv6.dport.high = 65535;

                if (cyg_httpd_form_varable_int(p, "sport_filter", &var_value3)) {
                    if (var_value3 != 0) {
                        if (cyg_httpd_form_varable_int(p, "sport_low", &var_value4)) {
                            newconf.frame.ipv6.sport.low = var_value4;
                            newconf.frame.ipv6.sport.high = var_value4;
                        }
                        if (cyg_httpd_form_varable_int(p, "sport_high", &var_value4)) {
                            newconf.frame.ipv6.sport.high = var_value4;
                        }
                    }
                }
                if (cyg_httpd_form_varable_int(p, "dport_filter", &var_value3)) {
                    if (var_value3 != 0) {
                        if (cyg_httpd_form_varable_int(p, "dport_low", &var_value4)) {
                            newconf.frame.ipv6.dport.low = var_value4;
                            newconf.frame.ipv6.dport.high = var_value4;
                        }
                        if (cyg_httpd_form_varable_int(p, "dport_high", &var_value4)) {
                            newconf.frame.ipv6.dport.high = var_value4;
                        }
                    }
                }

                //tcp_flags
                for (i = ACE_FLAG_TCP_FIN, j = 0; i < ACE_FLAG_COUNT; i++, j++) {
                    sprintf(var_name1, "tcp_flags%d", j);
                    if (cyg_httpd_form_varable_int(p, var_name1, &var_value3)) {
                        var_value3 = (var_value3 == 2 ? MESA_ACE_BIT_ANY : var_value3 == 0 ? MESA_ACE_BIT_0 : MESA_ACE_BIT_1);
                        switch (i) {
                        case ACE_FLAG_TCP_FIN:
                            newconf.frame.ipv6.tcp_fin = (mesa_ace_bit_t)var_value3;
                            break;
                        case ACE_FLAG_TCP_SYN:
                            newconf.frame.ipv6.tcp_syn = (mesa_ace_bit_t)var_value3;
                            break;
                        case ACE_FLAG_TCP_RST:
                            newconf.frame.ipv6.tcp_rst = (mesa_ace_bit_t)var_value3;
                            break;
                        case ACE_FLAG_TCP_PSH:
                            newconf.frame.ipv6.tcp_psh = (mesa_ace_bit_t)var_value3;
                            break;
                        case ACE_FLAG_TCP_ACK:
                            newconf.frame.ipv6.tcp_ack = (mesa_ace_bit_t)var_value3;
                            break;
                        case ACE_FLAG_TCP_URG:
                            newconf.frame.ipv6.tcp_urg = (mesa_ace_bit_t)var_value3;
                            break;
                        default:
                            break;
                        }
                    }
                }
                break;

            default:
                newconf.type = MESA_ACE_TYPE_IPV4;

                var_value2 = 0;
                (void) cyg_httpd_form_varable_int(p, "protocol_filter", &var_value2);

                if (var_value2 == 1) { //ICMP
                    newconf.frame.ipv4.proto.mask = 0xff;
                    newconf.frame.ipv4.proto.value = 1;

                    //icmp_type
                    if (cyg_httpd_form_varable_int(p, "icmp_type_filter", &var_value3)) {
                        if (var_value3 == 1) {
                            if (cyg_httpd_form_varable_int(p, "icmp_type", &var_value4)) {
                                newconf.frame.ipv4.data.mask[0] = 0xff;
                                newconf.frame.ipv4.data.value[0] = var_value4;
                            }
                        }
                    }

                    //icmp_code
                    if (cyg_httpd_form_varable_int(p, "icmp_code_filter", &var_value3)) {
                        if (var_value3 == 1) {
                            if (cyg_httpd_form_varable_int(p, "icmp_code", &var_value4)) {
                                newconf.frame.ipv4.data.mask[1] = 0xff;
                                newconf.frame.ipv4.data.value[1] = var_value4;
                            }
                        }
                    }
                } else if (var_value2 == 2) { //UDP
                    newconf.frame.ipv4.proto.mask = 0xff;
                    newconf.frame.ipv4.proto.value = 17;
                } else if (var_value2 == 3) { //TCP
                    newconf.frame.ipv4.proto.mask = 0xff;
                    newconf.frame.ipv4.proto.value = 6;
                } else { //Other
                    //protocol
                    if (cyg_httpd_form_varable_int(p, "protocol_filter", &var_value3)) {
                        if (var_value3 == 4) {
                            if (cyg_httpd_form_varable_int(p, "protocol", &var_value4)) {
                                newconf.frame.ipv4.proto.mask = 0xff;
                                newconf.frame.ipv4.proto.value = var_value4;
                            }
                        }
                    }
                }

                //sip, dip
                if (cyg_httpd_form_varable_int(p, "sip_filter", &var_value3)) {
                    if (var_value3 == 1 || var_value3 == 2) {
                        (void) cyg_httpd_form_varable_ipv4(p, "sip", &newconf.frame.ipv4.sip.value);
                        newconf.frame.ipv4.sip.mask = 0xFFFFFFFF;
                    }
                    if (var_value3 == 2) { //network
                        (void) cyg_httpd_form_varable_ipv4(p, "sip_mask", &newconf.frame.ipv4.sip.mask);
                    }
                }
                if (cyg_httpd_form_varable_int(p, "dip_filter", &var_value3)) {
                    if (var_value3 == 1 || var_value3 == 2) {
                        (void) cyg_httpd_form_varable_ipv4(p, "dip", &newconf.frame.ipv4.dip.value);
                        newconf.frame.ipv4.dip.mask = 0xFFFFFFFF;
                    }
                    if (var_value3 == 2) { //network
                        (void) cyg_httpd_form_varable_ipv4(p, "dip_mask", &newconf.frame.ipv4.dip.mask);
                    }
                }

                //ip flag
                for (i = ACE_FLAG_IP_TTL, j = 0; i < ACE_FLAG_TCP_FIN; i++, j++) {
                    sprintf(var_name1, "ip_flags%d", j);
                    if (cyg_httpd_form_varable_int(p, var_name1, &var_value3)) {
                        if (var_value3 <= 1) {
                            VTSS_BF_SET(newconf.flags.mask, i, 1);
                            VTSS_BF_SET(newconf.flags.value, i, var_value3);
                        }
                    }
                }

                //sport, dport range
                newconf.frame.ipv4.sport.in_range = newconf.frame.ipv4.dport.in_range = 1;
                newconf.frame.ipv4.sport.low = newconf.frame.ipv4.dport.low = 0;
                newconf.frame.ipv4.sport.high = newconf.frame.ipv4.dport.high = 65535;

                if (cyg_httpd_form_varable_int(p, "sport_filter", &var_value3)) {
                    if (var_value3 != 0) {
                        if (cyg_httpd_form_varable_int(p, "sport_low", &var_value4)) {
                            newconf.frame.ipv4.sport.low = var_value4;
                            newconf.frame.ipv4.sport.high = var_value4;
                        }
                        if (cyg_httpd_form_varable_int(p, "sport_high", &var_value4)) {
                            newconf.frame.ipv4.sport.high = var_value4;
                        }
                    }
                }
                if (cyg_httpd_form_varable_int(p, "dport_filter", &var_value3)) {
                    if (var_value3 != 0) {
                        if (cyg_httpd_form_varable_int(p, "dport_low", &var_value4)) {
                            newconf.frame.ipv4.dport.low = var_value4;
                            newconf.frame.ipv4.dport.high = var_value4;
                        }
                        if (cyg_httpd_form_varable_int(p, "dport_high", &var_value4)) {
                            newconf.frame.ipv4.dport.high = var_value4;
                        }
                    }
                }

                //tcp_flags
                for (i = ACE_FLAG_TCP_FIN, j = 0; i < ACE_FLAG_COUNT; i++, j++) {
                    sprintf(var_name1, "tcp_flags%d", j);
                    if (cyg_httpd_form_varable_int(p, var_name1, &var_value3)) {
                        if (var_value3 <= 1) {
                            VTSS_BF_SET(newconf.flags.mask, i, 1);
                            VTSS_BF_SET(newconf.flags.value, i, var_value3);
                        }
                    }
                }
                break;
            } //end of switch

            //save new configuration
            rc = acl_mgmt_ace_get(ACL_USER_STATIC, newconf.id, &ace_conf, NULL, 0);
            if (rc != VTSS_RC_OK || memcmp(&newconf, &ace_conf, sizeof(newconf)) != 0) {
                T_D("Calling acl_mgmt_ace_add(ACL_USER_STATIC, %d, %d)", next_ace_id, newconf.id);
                if ((rc = acl_mgmt_ace_add(ACL_USER_STATIC, next_ace_id, &newconf)) != VTSS_RC_OK) {
                    if (rc == VTSS_APPL_ACL_ERROR_ACE_TABLE_FULL) {
                        redirect_errmsg(p, "acl.htm", acl_error_txt(rc));
                    } else {
                        redirect_errmsg(p, new_url, acl_error_txt(rc));
                    }
                    return -1;
                }
            }
        }
        redirect(p, errors ? STACK_ERR_URL : "/acl.htm");
    } else { /* CYG_HTTPD_METHOD_GET (+HEAD) */
        /* Format: [flag]/[date]
           <Edit>        : 1/[eace_id]
           <Insert>      : 2/[next_id]
           <Add to Last> : 3/0
        */
        if ((var_string = cyg_httpd_form_varable_find(p, "aceConfigFlag")) != NULL) {
            ace_flag = atoi(var_string);
            if ((var_string = cyg_httpd_form_varable_find(p, "SelectAceId")) != NULL) {
                var_value2 = atoi(var_string);
            }
            switch (ace_flag) {
            case 1:
                ace_id = (mesa_ace_id_t) var_value2;
                break;
            case 2:
                next_ace_id = (mesa_ace_id_t) var_value2;
                break;
            case 3:
                break;
            default:
                break;
            }
        }

        (void)cyg_httpd_start_chunked("html");


        (void)cyg_httpd_write_chunked(",", 1);

        /* get form data
           Format: [sid]#[sid]#...,[isid]/[ace_id]/[next_ace_id]/[action]/[filter_port_1]:[filter_port_2]:.../[lookup]/[ingress_port]/[policy_filter]/[policy]/[policy_bitmask]/[rate_limiter]/0/0/[port_copy_1]:[port_copy_2]:.../[mirror]/[logging]/[shutdown]/[dmac_filter]/[dmac_mac]/[tagged]/[vid_mask]/[vid]/[user_prio]/[vlan_cfi]/[counters]/[frame_type]/[frame_type_filed]|...
           [frame_type_filed]
           - [frame_type] = Any
           - [frame_type] = Ethernet [smac_filter]/[smac]/[ether_type_filter]/[ether_type]
           - [frame_type] = ARP      [smac_filter]/[smac]/[arp_flags0]/[arp_flags2]/[arp_sip_filter]/[arp_sip]/[arp_sip_mask]/[arp_dip_filter]/[arp_dip]/[arp_dip_mask]/[arp_flags3]/[arp_flags4]/[arp_flags5]/[arp_flags6]/[arp_flags7]
           - [frame_type] = IPv4, [protocol] = Any/Other [protocol_filter]/[protocol]/[ip_flags0]/[ip_flags1]/[ip_flags2]/[sip_filter]/[sip]/[sip_mask]/[dip_filter]/[dip]/[dip_mask]
           - [frame_type] = IPv4, [protocol] = ICMP      [protocol_filter]/[protocol]/[ip_flags0]/[ip_flags1]/[ip_flags2]/[sip_filter]/[sip]/[sip_mask]/[dip_filter]/[dip]/[dip_mask]/[icmp_type_filter]/[icmp_type]/[icmp_code_filter]/[icmp_code]
           - [frame_type] = IPv4, [protocol] = UDP       [protocol_filter]/[protocol]/[ip_flags0]/[ip_flags1]/[ip_flags2]/[sip_filter]/[sip]/[sip_mask]/[dip_filter]/[dip]/[dip_mask]/[sport_filter]/[sport_low]/[sport_high]/[dport_filter]/[dport_low]/[dport_high]
           - [frame_type] = IPv4, [protocol] = TCP       [protocol_filter]/[protocol]/[ip_flags0]/[ip_flags1]/[ip_flags2]/[sip_filter]/[sip]/[sip_mask]/[dip_filter]/[dip]/[dip_mask]/[sport_filter]/[sport_low]/[sport_high]/[dport_filter]/[dport_low]/[dport_high]/[tcp_flags0]/[tcp_flags1]/[tcp_flags2]/[tcp_flags3]/[tcp_flags4]/[tcp_flags5]
           - [frame_type] = IPv6  [protocol] = Any/Other [next_header_filter]/[next_header]/[sip_v6_filter]/[sip_v6]/[sip_v6_mask]/[hop_limit]
           - [frame_type] = IPv6  [protocol] = ICMP      [next_header_filter]/[next_header]/[sip_v6_filter]/[sip_v6]/[sip_v6_mask]/[hop_limit]/[dummy_field_1]..[dummy_field_5]/[icmp_type_filter]/[icmp_type]/[icmp_code_filter]/[icmp_code]
           - [frame_type] = IPv6  [protocol] = UDP       [next_header_filter]/[next_header]/[sip_v6_filter]/[sip_v6]/[sip_v6_mask]/[hop_limit]/[dummy_field_1]..[dummy_field_5]/[sport_filter]/[sport_low]/[sport_high]/[dport_filter]/[dport_low]/[dport_high]
           - [frame_type] = IPv6  [protocol] = TCP       [next_header_filter]/[next_header]/[sip_v6_filter]/[sip_v6]/[sip_v6_mask]/[hop_limit]/[dummy_field_1]..[dummy_field_5]/[sport_filter]/[sport_low]/[sport_high]/[dport_filter]/[dport_low]/[dport_high]/[tcp_flags0]/[tcp_flags1]/[tcp_flags2]/[tcp_flags3]/[tcp_flags4]/[tcp_flags5]
        */
        if ((ace_flag == 1) && (acl_mgmt_ace_get(ACL_USER_STATIC, ace_id, &ace_conf, &ace_counter, 0) == VTSS_RC_OK)) {
            rc = acl_mgmt_ace_get(ACL_USER_STATIC, ace_conf.id, &newconf, NULL, 1);

            //dmac_filter
            var_value2 = VTSS_BF_GET(ace_conf.flags.mask, ACE_FLAG_DMAC_BC) ? (VTSS_BF_GET(ace_conf.flags.value, ACE_FLAG_DMAC_BC) ? 1 : 0) : 2;
            var_value3 = VTSS_BF_GET(ace_conf.flags.mask, ACE_FLAG_DMAC_MC) ? (VTSS_BF_GET(ace_conf.flags.value, ACE_FLAG_DMAC_MC) ? 1 : 0) : 2;
            if (var_value2 == 2 && var_value3 == 2) {
                var_value1 = 0; //any
            } else if (var_value2 == 0 && var_value3 == 1) {
                var_value1 = 1; //MC
            } else if (var_value2 == 1 && var_value3 == 1) {
                var_value1 = 2; //BC
            } else if (var_value2 == 0 && var_value3 == 0) {
                var_value1 = 3; //UC
            }

            //dmac
            any = 1;
            for (i = 0; i < 6; i++) {
                if (ace_conf.frame.etype.dmac.mask[i]) {
                    any = 0;
                    break;
                }
            }
            if (any || (ace_conf.type != MESA_ACE_TYPE_ETYPE)) {
                strcpy(str_buff1, "00-00-00-00-00-02");
            } else {
                var_value1 = 4; //Specific
                (void) misc_mac_txt(ace_conf.frame.etype.dmac.value, str_buff1);
            }

            for (port_list_cnt = 0, port_filter_list_cnt = 0, i = 0; i < port_count; i++) {
                if (ace_conf.port_list[i]) {
                    port_list_cnt++;
                }
                if (ace_conf.action.port_list[i]) {
                    port_filter_list_cnt++;
                }
            }
            if (ace_conf.action.port_action == MESA_ACL_PORT_ACTION_FILTER && port_filter_list_cnt != 0) {
                is_filter_enable = TRUE;
            } else {
                is_filter_enable = FALSE;
            }

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/%u/%u/%u",
                          VTSS_ISID_LOCAL,
                          ace_conf.id,
                          rc == VTSS_RC_OK ? newconf.id : ACL_MGMT_ACE_ID_NONE,
                          ace_conf.action.port_action == MESA_ACL_PORT_ACTION_NONE ? 1 : is_filter_enable ? 2 : 0
                         );
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            //filter_port
            if (is_filter_enable && port_filter_list_cnt != port_count) {
                buf[0] = '\0';
                is_first = 1;
                for (i = 0; i < port_count; i++) {
                    if (ace_conf.action.port_list[i]) {
                        if (is_first) {
                            is_first = 0;
                            sprintf(buf + strlen(buf), "%u", iport2uport(i));
                        } else {
                            sprintf(buf + strlen(buf), ":%u", iport2uport(i));
                        }
                    }
                }
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s",  buf);
            } else {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/0");
            }
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d", ace_conf.lookup);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            //ingress_port
            if (port_list_cnt == port_count) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/0");
            } else {
                buf[0] = '\0';
                is_first = 1;
                for (i = 0; i < port_count; i++) {
                    if (ace_conf.port_list[i]) {
                        if (is_first) {
                            is_first = 0;
                            sprintf(buf + strlen(buf), "%u", iport2uport(i));
                        } else {
                            sprintf(buf + strlen(buf), ":%u", iport2uport(i));
                        }
                    }
                }
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s",  buf);
            }
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            // policy_filter, policy, policy_bitmask
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/%u/%X",
                          ace_conf.policy.mask == 0x0 ? 0 : 1,
                          ace_conf.policy.value,
                          ace_conf.policy.mask ? ace_conf.policy.mask : ACL_MGMT_POLICIES_BITMASK);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u", ace_conf.action.policer == ACL_MGMT_RATE_LIMITER_NONE ? 0 : ipolicer2upolicer(ace_conf.action.policer));
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            // Left-over from earlier versions. Must be here.
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/0/0");
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            buf[0] = '\0';
            if (ace_conf.action.port_action == MESA_ACL_PORT_ACTION_REDIR) {
                is_first = 1;
                for (i = 0; i < port_count; i++) {
                    if (ace_conf.action.port_list[i]) {
                        if (is_first) {
                            is_first = 0;
                            sprintf(buf + strlen(buf), "%u", iport2uport(i));
                        } else {
                            sprintf(buf + strlen(buf), ":%u", iport2uport(i));
                        }
                    }
                }
            } else {
                // port_copy disabled
                sprintf(buf + strlen(buf), "0");
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s/%u",
                          buf,
                          ace_conf.action.mirror);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/%u/%u/%s/%u/%u/%u/%u/%u/%u",
                          ace_conf.action.logging,
                          ace_conf.action.shutdown,
                          var_value1,
                          str_buff1,
                          ace_conf.tagged,
                          ace_conf.vid.mask == 0 ? 0 : 1,
                          ace_conf.vid.value,
                          acl_tag_prio_range_mapping(ace_conf.usr_prio),
                          VTSS_BF_GET(ace_conf.flags.mask, ACE_FLAG_VLAN_CFI) ? (VTSS_BF_GET(ace_conf.flags.value, ACE_FLAG_VLAN_CFI) ? 1 : 0) : 2,
                          ace_counter);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            if (ace_conf.type == MESA_ACE_TYPE_ANY) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/0");
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            } else if (ace_conf.type == MESA_ACE_TYPE_ETYPE) {

                any = 1;
                for (i = 0; i < 6; i++) {
                    if (ace_conf.frame.etype.smac.mask[i]) {
                        any = 0;
                        break;
                    }
                }
                if (any) {
                    strcpy(str_buff1, "1/0/00-00-00-00-00-01");
                } else {
                    strcpy(str_buff1, "1/1/");
                    (void) misc_mac_txt(ace_conf.frame.etype.smac.value, &str_buff1[4]);
                }

                if (ace_conf.frame.etype.etype.mask[0] || ace_conf.frame.etype.etype.mask[1]) {
                    sprintf(str_buff2, "1/%02x%02x", ace_conf.frame.etype.etype.value[0], ace_conf.frame.etype.etype.value[1]);
                } else {
                    sprintf(str_buff2, "0/0");
                }

                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s/%s", str_buff1, str_buff2);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            } else if (ace_conf.type == MESA_ACE_TYPE_ARP) {

                any = 1;
                for (i = 0; i < 6; i++) {
                    if (ace_conf.frame.arp.smac.mask[i]) {
                        any = 0;
                        break;
                    }
                }
                if (any) {
                    strcpy(str_buff1, "2/0/00-00-00-00-00-01");
                } else {
                    strcpy(str_buff1, "2/1/");
                    (void) misc_mac_txt(ace_conf.frame.arp.smac.value, &str_buff1[4]);
                }

                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s/%u/%u/%u/%s/%s/%u/%s/%s/%u/%u/%u/%u/%u",
                              str_buff1,
                              VTSS_BF_GET(ace_conf.flags.mask, ACE_FLAG_ARP_ARP) ? (VTSS_BF_GET(ace_conf.flags.value, ACE_FLAG_ARP_ARP) ? 1 : 0) : VTSS_BF_GET(ace_conf.flags.mask, ACE_FLAG_ARP_UNKNOWN) ? 3 : 2,
                              VTSS_BF_GET(ace_conf.flags.mask, ACE_FLAG_ARP_REQ) ? (VTSS_BF_GET(ace_conf.flags.value, ACE_FLAG_ARP_REQ) ? 1 : 0) : 2,
                              ace_conf.frame.arp.sip.mask != 0 ? (ace_conf.frame.arp.sip.mask == 0xFFFFFFFF ? 1 : 2 ) : 0,
                              misc_ipv4_txt(ace_conf.frame.arp.sip.value, str_buff2),
                              misc_ipv4_txt(ace_conf.frame.arp.sip.mask, str_buff3),
                              ace_conf.frame.arp.dip.mask != 0 ? (ace_conf.frame.arp.dip.mask == 0xFFFFFFFF ? 1 : 2 ) : 0,
                              misc_ipv4_txt(ace_conf.frame.arp.dip.value, str_buff4),
                              misc_ipv4_txt(ace_conf.frame.arp.dip.mask, str_buff5),
                              VTSS_BF_GET(ace_conf.flags.mask, ACE_FLAG_ARP_SMAC) ? (VTSS_BF_GET(ace_conf.flags.value, ACE_FLAG_ARP_SMAC) ? 1 : 0) : 2,
                              VTSS_BF_GET(ace_conf.flags.mask, ACE_FLAG_ARP_DMAC) ? (VTSS_BF_GET(ace_conf.flags.value, ACE_FLAG_ARP_DMAC) ? 1 : 0) : 2,
                              VTSS_BF_GET(ace_conf.flags.mask, ACE_FLAG_ARP_LEN) ? (VTSS_BF_GET(ace_conf.flags.value, ACE_FLAG_ARP_LEN) ? 1 : 0) : 2,
                              VTSS_BF_GET(ace_conf.flags.mask, ACE_FLAG_ARP_IP) ? (VTSS_BF_GET(ace_conf.flags.value, ACE_FLAG_ARP_IP) ? 1 : 0) : 2,
                              VTSS_BF_GET(ace_conf.flags.mask, ACE_FLAG_ARP_ETHER) ? (VTSS_BF_GET(ace_conf.flags.value, ACE_FLAG_ARP_ETHER) ? 1 : 0) : 2);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            } else if (ace_conf.type == MESA_ACE_TYPE_IPV4) {

                icmp = (ace_conf.frame.ipv4.proto.value == 1);
                udp = (ace_conf.frame.ipv4.proto.value == 17);
                tcp = (ace_conf.frame.ipv4.proto.value == 6);

                if (ace_conf.frame.ipv4.proto.mask) {
                    if (icmp) { //ICMP
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/3/1/%u", ace_conf.frame.ipv4.proto.value);
                        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                    } else if (udp) {
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/3/2/%u", ace_conf.frame.ipv4.proto.value);
                        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                    } else if (tcp) {
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/3/3/%u", ace_conf.frame.ipv4.proto.value);
                        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                    } else {
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/3/4/%u", ace_conf.frame.ipv4.proto.value);
                        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                    }
                } else { //Any
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/3/0/%u", ace_conf.frame.ipv4.proto.value);
                    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                }

                //ip_flags
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/%u/%u",
                              VTSS_BF_GET(ace_conf.flags.mask, ACE_FLAG_IP_TTL) ? (VTSS_BF_GET(ace_conf.flags.value, ACE_FLAG_IP_TTL) ? 1 : 0) : 2,
                              VTSS_BF_GET(ace_conf.flags.mask, ACE_FLAG_IP_FRAGMENT) ? (VTSS_BF_GET(ace_conf.flags.value, ACE_FLAG_IP_FRAGMENT) ? 1 : 0) : 2,
                              VTSS_BF_GET(ace_conf.flags.mask, ACE_FLAG_IP_OPTIONS) ? (VTSS_BF_GET(ace_conf.flags.value, ACE_FLAG_IP_OPTIONS) ? 1 : 0) : 2);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);

                //sip, dip
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/%s/%s/%u/%s/%s",
                              ace_conf.frame.ipv4.sip.mask != 0 ? (ace_conf.frame.ipv4.sip.mask == 0xFFFFFFFF ? 1 : 2 ) : 0,
                              misc_ipv4_txt(ace_conf.frame.ipv4.sip.value, str_buff1),
                              misc_ipv4_txt(ace_conf.frame.ipv4.sip.mask, str_buff2),
                              ace_conf.frame.ipv4.dip.mask != 0 ? (ace_conf.frame.ipv4.dip.mask == 0xFFFFFFFF ? 1 : 2 ) : 0,
                              misc_ipv4_txt(ace_conf.frame.ipv4.dip.value, str_buff3),
                              misc_ipv4_txt(ace_conf.frame.ipv4.dip.mask, str_buff4));
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);

                if (icmp) {
                    //icmp_type, icmp_code
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/%u/%u/%u",
                                  ace_conf.frame.ipv4.data.mask[0] == 0 ? 0 : 1,
                                  ace_conf.frame.ipv4.data.value[0],
                                  ace_conf.frame.ipv4.data.mask[1] == 0 ? 0 : 1,
                                  ace_conf.frame.ipv4.data.value[1]);
                    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                }

                //sport_range, dport_range
                if (udp || tcp) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/%d/%d/%u/%d/%d",
                                  (ace_conf.frame.ipv4.sport.low == 0 && ace_conf.frame.ipv4.sport.high == 65535) ? 0 : ace_conf.frame.ipv4.sport.low == ace_conf.frame.ipv4.sport.high ? 1 : 2,
                                  ace_conf.frame.ipv4.sport.low,
                                  ace_conf.frame.ipv4.sport.high,
                                  (ace_conf.frame.ipv4.dport.low == 0 && ace_conf.frame.ipv4.dport.high == 65535) ? 0 : ace_conf.frame.ipv4.dport.low == ace_conf.frame.ipv4.dport.high ? 1 : 2,
                                  ace_conf.frame.ipv4.dport.low,
                                  ace_conf.frame.ipv4.dport.high);
                    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                }

                //tcp_flags
                if (tcp) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/%u/%u/%u/%u/%u",
                                  VTSS_BF_GET(ace_conf.flags.mask, ACE_FLAG_TCP_FIN) ? (VTSS_BF_GET(ace_conf.flags.value, ACE_FLAG_TCP_FIN) ? 1 : 0) : 2,
                                  VTSS_BF_GET(ace_conf.flags.mask, ACE_FLAG_TCP_SYN) ? (VTSS_BF_GET(ace_conf.flags.value, ACE_FLAG_TCP_SYN) ? 1 : 0) : 2,
                                  VTSS_BF_GET(ace_conf.flags.mask, ACE_FLAG_TCP_RST) ? (VTSS_BF_GET(ace_conf.flags.value, ACE_FLAG_TCP_RST) ? 1 : 0) : 2,
                                  VTSS_BF_GET(ace_conf.flags.mask, ACE_FLAG_TCP_PSH) ? (VTSS_BF_GET(ace_conf.flags.value, ACE_FLAG_TCP_PSH) ? 1 : 0) : 2,
                                  VTSS_BF_GET(ace_conf.flags.mask, ACE_FLAG_TCP_ACK) ? (VTSS_BF_GET(ace_conf.flags.value, ACE_FLAG_TCP_ACK) ? 1 : 0) : 2,
                                  VTSS_BF_GET(ace_conf.flags.mask, ACE_FLAG_TCP_URG) ? (VTSS_BF_GET(ace_conf.flags.value, ACE_FLAG_TCP_URG) ? 1 : 0) : 2);
                    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                }
            } else if (ace_conf.type == MESA_ACE_TYPE_IPV6) {
                int sipv6_filter = 0;
                char ipv6_str_buf[40];
                mesa_ipv6_t ipv6_addr;

                icmp = (ace_conf.frame.ipv6.proto.value == 58);
                udp = (ace_conf.frame.ipv6.proto.value == 17);
                tcp = (ace_conf.frame.ipv6.proto.value == 6);

                memset(&ipv6_addr, 0, sizeof(ipv6_addr));
                for (i = 0; i < 16; i++) {
                    if (ace_conf.frame.ipv6.sip.mask[i]) {
                        sipv6_filter = 1;
                        memcpy(&ipv6_addr, ace_conf.frame.ipv6.sip.value, sizeof(ipv6_addr));
                        break;
                    }
                }
                sip_v6_mask = (ace_conf.frame.ipv6.sip.mask[12] << 24) |
                              (ace_conf.frame.ipv6.sip.mask[13] << 16) |
                              (ace_conf.frame.ipv6.sip.mask[14] << 8)  |
                              ace_conf.frame.ipv6.sip.mask[15];
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/4/%d/%d/%d/%s/%X",
                              ace_conf.frame.ipv6.proto.mask ? icmp ? 1 : udp ? 2 : tcp ? 3 : 4 : 0,
                              ace_conf.frame.ipv6.proto.mask ? ace_conf.frame.ipv6.proto.value : 0,
                              sipv6_filter,
                              sipv6_filter ? misc_ipv6_txt(&ipv6_addr, ipv6_str_buf) : " ",
                              sip_v6_mask);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);

                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d",
                              ace_conf.frame.ipv6.ttl == MESA_ACE_BIT_ANY ? 2 : ace_conf.frame.ipv6.ttl == MESA_ACE_BIT_0 ? 0 : 1);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);

                if (icmp || udp || tcp) {
                    //[dummy_field_1]..[dummy_field_5]
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/0/0/0/0/0");
                    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                }

                if (icmp) {
                    //icmp_type, icmp_code
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/%u/%u/%u",
                                  ace_conf.frame.ipv6.data.mask[0] == 0 ? 0 : 1,
                                  ace_conf.frame.ipv6.data.value[0],
                                  ace_conf.frame.ipv6.data.mask[1] == 0 ? 0 : 1,
                                  ace_conf.frame.ipv6.data.value[1]);
                    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                }

                //sport_range, dport_range
                if (udp || tcp) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/%d/%d/%u/%d/%d",
                                  (ace_conf.frame.ipv6.sport.low == 0 && ace_conf.frame.ipv6.sport.high == 65535) ? 0 : ace_conf.frame.ipv6.sport.low == ace_conf.frame.ipv6.sport.high ? 1 : 2,
                                  ace_conf.frame.ipv6.sport.low,
                                  ace_conf.frame.ipv6.sport.high,
                                  (ace_conf.frame.ipv6.dport.low == 0 && ace_conf.frame.ipv6.dport.high == 65535) ? 0 : ace_conf.frame.ipv6.dport.low == ace_conf.frame.ipv6.dport.high ? 1 : 2,
                                  ace_conf.frame.ipv6.dport.low,
                                  ace_conf.frame.ipv6.dport.high);
                    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                }

                //tcp_flags
                if (tcp) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u/%u/%u/%u/%u/%u",
                                  ace_conf.frame.ipv6.tcp_fin == MESA_ACE_BIT_ANY ? 2 : ace_conf.frame.ipv6.tcp_fin == MESA_ACE_BIT_0 ? 0 : 1,
                                  ace_conf.frame.ipv6.tcp_syn == MESA_ACE_BIT_ANY ? 2 : ace_conf.frame.ipv6.tcp_syn == MESA_ACE_BIT_0 ? 0 : 1,
                                  ace_conf.frame.ipv6.tcp_rst == MESA_ACE_BIT_ANY ? 2 : ace_conf.frame.ipv6.tcp_rst == MESA_ACE_BIT_0 ? 0 : 1,
                                  ace_conf.frame.ipv6.tcp_psh == MESA_ACE_BIT_ANY ? 2 : ace_conf.frame.ipv6.tcp_psh == MESA_ACE_BIT_0 ? 0 : 1,
                                  ace_conf.frame.ipv6.tcp_ack == MESA_ACE_BIT_ANY ? 2 : ace_conf.frame.ipv6.tcp_ack == MESA_ACE_BIT_0 ? 0 : 1,
                                  ace_conf.frame.ipv6.tcp_urg == MESA_ACE_BIT_ANY ? 2 : ace_conf.frame.ipv6.tcp_urg == MESA_ACE_BIT_0 ? 0 : 1);
                    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                }
            }
        } else {
            //default ACE setting
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/0/%u/1/0/0/0/0/0/%x/0/0/1/0/0/0/0/0/00-00-00-00-00-02/0/0/1/14/2/0/0", VTSS_USID_END, next_ace_id, ACL_MGMT_POLICIES_BITMASK);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_status_acl(CYG_HTTPD_STATE *p)
{
    int                 ct;
    mesa_rc             rc;
    mesa_ace_id_t       ace_id = ACL_MGMT_ACE_ID_NONE;
    acl_entry_conf_t    ace_conf, newconf;
    mesa_ace_counter_t  ace_counter;
    int                 acl_user = -1;
    int                 i;
    int                 acl_user_idx;
    const char          *var_string;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_GET) { /* CYG_HTTPD_METHOD_GET (+HEAD) */
        if ((var_string = cyg_httpd_form_varable_find(p, "aclUser")) != NULL) {
            acl_user = atoi(var_string);    /* From request url (xyz.htm?aclUser=zz) */
        }

        /* get form data
           Format: [select_user_id]/[user_name1]/[user_name2]/...,[user_id]/[copy_to_cpu]/[hit_me_once]/[conflict]/[isid]/[ace_id]/[next_ace_id]/[action]/[ingress_port]/[rate_limiter]/[port_copy_1]:[port_copy_2]:.../[mirror]/[logging]/[shutdown]/[dmac_filter]/[dmac_mac]/[counters]/[frame_type]/[frame_type_filed]|...
           [frame_type_filed]
           - [frame_type] = Any
           - [frame_type] = Ethernet [smac_filter]/[smac]/[ether_type_filter]/[ether_type]
           - [frame_type] = ARP      [smac_filter]/[smac]/[arp_sip_filter]/[arp_sip]/[arp_sip_mask]/[arp_dip_filter]/[arp_dip]/[arp_dip_mask]
           - [frame_type] = IPv4, [protocol] = Any/Other [protocol_filter]/[protocol]/[ip_flags1]/[sip_filter]/[sip]/[sip_mask]/[dip_filter]/[dip]/[dip_mask]
           - [frame_type] = IPv4, [protocol] = ICMP      [protocol_filter]/[protocol]/[ip_flags1]/[sip_filter]/[sip]/[sip_mask]/[dip_filter]/[dip]/[dip_mask]/[icmp_type_filter]/[icmp_type]/[icmp_code_filter]/[icmp_code]
           - [frame_type] = IPv4, [protocol] = UDP       [protocol_filter]/[protocol]/[ip_flags1]/[sip_filter]/[sip]/[sip_mask]/[dip_filter]/[dip]/[dip_mask]/[sport_filter]/[sport_low]/[sport_high]/[dport_filter]/[dport_low]/[dport_high]
           - [frame_type] = IPv4, [protocol] = TCP       [protocol_filter]/[protocol]/[ip_flags1]/[sip_filter]/[sip]/[sip_mask]/[dip_filter]/[dip]/[dip_mask]/[sport_filter]/[sport_low]/[sport_high]/[dport_filter]/[dport_low]/[dport_high]/[tcp_flags0]/[tcp_flags1]/[tcp_flags2]/[tcp_flags3]/[tcp_flags4]/[tcp_flags5]
           - [frame_type] = IPv6  [next_header_filter]/[next_header]/[sip_v6_filter]/[sip_v6]/[sip_v6_mask]
        */
        (void)cyg_httpd_start_chunked("html");

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/", acl_user);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        for (i = ACL_USER_STATIC; i < ACL_USER_CNT; ++i) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s/", acl_mgmt_user_name_get((acl_user_t)i));
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        (void)cyg_httpd_write_chunked(",", 1);

        /* Show user ACEs */
        if (acl_user < 0) {
            for (acl_user_idx = ACL_USER_CNT - 1; acl_user_idx >= ACL_USER_STATIC; acl_user_idx--) {
                ace_id = ACL_MGMT_ACE_ID_NONE;
                while (acl_mgmt_ace_get((acl_user_t)acl_user_idx, ace_id, &ace_conf, &ace_counter, 1) == VTSS_RC_OK) {
                    rc = acl_mgmt_ace_get((acl_user_t)acl_user_idx, ace_conf.id, &newconf, NULL, 1);
                    ace_id = ace_conf.id;
                    if (acl_user == -1 || (acl_user == -2 && ace_conf.conflict)) {
                        ace_overview(p, (acl_user_t)acl_user_idx, rc == VTSS_RC_OK ? newconf.id : ACL_MGMT_ACE_ID_NONE, &ace_conf, ace_counter, 1);
                    }
                }
            }
        } else {
            ace_id = ACL_MGMT_ACE_ID_NONE;
            while (acl_mgmt_ace_get((acl_user_t)acl_user, ace_id, &ace_conf, &ace_counter, 1) == VTSS_RC_OK) {
                rc = acl_mgmt_ace_get((acl_user_t)acl_user, ace_conf.id, &newconf, NULL, 1);
                ace_id = ace_conf.id;
                ace_overview(p, (acl_user_t)acl_user, rc == VTSS_RC_OK ? newconf.id : ACL_MGMT_ACE_ID_NONE, &ace_conf, ace_counter, 1);
            }
        }
        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_config_acl_ports(CYG_HTTPD_STATE *p)
{
    vtss_isid_t     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    mesa_port_no_t  iport;
    vtss_uport_no_t uport;
    int             ct;
    acl_port_conf_t port_conf, newconf;
    char            var_name[24];
    int             var_value;
    u32             port_count = port_count_max();
    mesa_acl_port_counter_t counter;
    const char      *var_string;
    int             i, is_first;
    char            buf[80];
    port_vol_conf_t port_state_conf, port_state_newconf;
    port_user_t     user = PORT_USER_ACL;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;
        /* store form data */
        for (iport = 0; iport < port_count; iport++) {
            memset(&port_state_conf, 0, sizeof(port_state_conf));
            if (acl_mgmt_port_conf_get(iport, &port_conf) != VTSS_RC_OK ||
                (port_vol_conf_get(user, iport, &port_state_conf) != VTSS_RC_OK)) {
                T_E("acl_mgmt_port_conf_get(%u): failed", iport);
                errors++; /* Probably stack error */
                continue;
            }
            uport = iport2uport(iport);
            newconf = port_conf;
            port_state_newconf = port_state_conf;

            //policy_id
            sprintf(var_name, "policy_id%d", uport);
            if (cyg_httpd_form_varable_int(p, var_name, &var_value)) {
                newconf.policy_no = var_value;
            }

            //rate_limiter
            sprintf(var_name, "rate_limiter_id%d", uport);
            if (cyg_httpd_form_varable_int(p, var_name, &var_value)) {
                newconf.action.policer = (var_value == 0 ? ACL_MGMT_RATE_LIMITER_NONE : upolicer2ipolicer(var_value));
            }

            //action
            sprintf(var_name, "action%d", uport);
            if (cyg_httpd_form_varable_int(p, var_name, &var_value)) {
                newconf.action.port_action = (var_value ? MESA_ACL_PORT_ACTION_NONE : MESA_ACL_PORT_ACTION_FILTER);
                //port_count = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);
                for (i = 0; i < port_count; i++) {
                    newconf.action.port_list[i] = var_value;
                }
            }

            //port_copy
            i = 1;
            sprintf(var_name, "port_copy_no%d", uport);
            while (cyg_httpd_form_multi_varable_int(p, var_name, &var_value, i++)) {
                if (var_value == 0) { /* Disabled */
                    break;
                } else {
                    if (i == 1) {
                        memset(newconf.action.port_list, 0, sizeof(newconf.action.port_list));
                    }
                    newconf.action.port_action = MESA_ACL_PORT_ACTION_REDIR;
                    newconf.action.port_list[uport2iport(var_value)] = TRUE;
                }
            }

            //mirror
            sprintf(var_name, "mirror%d", uport);
            if (cyg_httpd_form_varable_int(p, var_name, &var_value)) {
                newconf.action.mirror = var_value;
            }

            //logging
            sprintf(var_name, "logging%d", uport);
            if (cyg_httpd_form_varable_int(p, var_name, &var_value)) {
                newconf.action.logging = var_value;
            }

            //shutdown
            sprintf(var_name, "shutdown%d", uport);
            if (cyg_httpd_form_varable_int(p, var_name, &var_value)) {
                newconf.action.shutdown = var_value;
            }

            //state
            sprintf(var_name, "state%d", uport);
            if (cyg_httpd_form_varable_int(p, var_name, &var_value)) {
                port_state_newconf.disable = !var_value;
            }

            if (memcmp(&newconf, &port_conf, sizeof(newconf)) != 0) {
                T_D("Calling acl_mgmt_port_conf_set(%d)", iport);
                if (acl_mgmt_port_conf_set(iport, &newconf) != VTSS_RC_OK) {
                    errors++;
                    T_E("acl_mgmt_port_conf_set(%d): failed", iport);
                }
            }

            if (memcmp(&port_state_newconf, &port_state_conf, sizeof(port_state_newconf)) != 0) {
                T_D("Calling port_vol_conf_set(%u)", iport);
                if (port_vol_conf_set(user, iport, &port_state_newconf) != VTSS_RC_OK) {
                    errors++;
                    T_E("port_vol_conf_set(%u): failed", iport);
                }
            }
        }

        redirect(p, errors ? STACK_ERR_URL : "/acl_ports.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        if ((var_string = cyg_httpd_form_varable_find(p, "aceConfigFlag")) != NULL) {
            var_value = atoi(var_string);
            if (var_value == 1) {
                if (acl_mgmt_counters_clear() != VTSS_RC_OK) {
                    T_W("acl_mgmt_counters_clear() failed");
                }
            }
        }

        (void)cyg_httpd_start_chunked("html");

        /* get form data
           Format: [uport]/[action]/[rate_limiter]/[port_copy_1]:[port_copy_2]:.../[mirror]/[logging]/[shutdown]/[state]/[counter]|...
        */
        memset(&port_state_conf, 0, sizeof(port_state_conf));
        for (iport = 0; iport < port_count; iport++) {
            if (acl_mgmt_port_conf_get(iport, &port_conf)  != VTSS_RC_OK ||
                acl_mgmt_port_counter_get(iport, &counter) != VTSS_RC_OK ||
                ( port_vol_conf_get(user, iport, &port_state_conf) != VTSS_RC_OK)) {
                /* Bail out */
                continue;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                          "%u/%u/%u/%u",
                          iport2uport(iport),
                          port_conf.policy_no,
                          port_conf.action.port_action == MESA_ACL_PORT_ACTION_NONE,
                          port_conf.action.policer == ACL_MGMT_RATE_LIMITER_NONE ? 0 : ipolicer2upolicer(port_conf.action.policer));
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            //port_copy
            buf[0] = '\0';
            if (port_conf.action.port_action == MESA_ACL_PORT_ACTION_REDIR) {
                is_first = 1;
                //port_count = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);
                for (i = 0; i < port_count; i++) {
                    if (port_conf.action.port_list[i]) {
                        if (is_first) {
                            is_first = 0;
                            sprintf(buf + strlen(buf), "%u", iport2uport(i));
                        } else {
                            sprintf(buf + strlen(buf), ":%u", iport2uport(i));
                        }
                    }
                }
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                          "/%s/%u",
                          strlen(buf) == 0 ? "0" : buf,
                          port_conf.action.mirror);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                          "/%u/%u/%u/%u|",
                          port_conf.action.logging,
                          port_conf.action.shutdown,
                          !port_state_conf.disable,
                          counter);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}


static i32 handler_config_acl_ratelimiter(CYG_HTTPD_STATE *p)
{
    int                                     ct;
    mesa_acl_policer_no_t                   policer_no;
    vtss_appl_acl_config_rate_limiter_t     policer_conf, newconf;
    char                                    var_name[32];
    int                                     var_value;
    int                                     rate_unit;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        /* store form data */
        for (policer_no = 0; policer_no < ACL_MGMT_RATE_LIMITER_NO_END; policer_no++) {
            if (acl_mgmt_policer_conf_get(policer_no, &policer_conf) != VTSS_RC_OK) {
                continue;
            }
            newconf = policer_conf;

            sprintf(var_name, "unit_%d", ipolicer2upolicer(policer_no));
            if (cyg_httpd_form_varable_int(p, var_name, &rate_unit)) {
                sprintf(var_name, "rate_%d", ipolicer2upolicer(policer_no));
                if (cyg_httpd_form_varable_int(p, var_name, &var_value)) {
                    if (rate_unit == 0) { //PPS
                        newconf.bit_rate_enable = FALSE;
                        newconf.packet_rate = var_value;
                    } else { //KBPS
                        newconf.bit_rate_enable = TRUE;
                        newconf.bit_rate = var_value;
                    }
                }
            }

            if (memcmp(&newconf, &policer_conf, sizeof(newconf)) != 0) {
                T_D("Calling acl_mgmt_policer_conf_set(%d)", policer_no);
                if (acl_mgmt_policer_conf_set(policer_no, &newconf) < 0) {
                    T_E("acl_mgmt_policer_conf_set(%d): failed", policer_no);
                }
            }
        }
        redirect(p, "/acl_ratelimiter.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        (void)cyg_httpd_start_chunked("html");

        /* get form data
           Format: [rate_limiter_id]/[rate]/[rate_unit]|...
        */
        for (policer_no = 0; policer_no < ACL_MGMT_RATE_LIMITER_NO_END; policer_no++) {
            if (acl_mgmt_policer_conf_get(policer_no, &policer_conf) != VTSS_RC_OK) {
                continue;
            }

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%u/%d|",
                          ipolicer2upolicer(policer_no),
                          policer_conf.bit_rate_enable ? policer_conf.bit_rate : policer_conf.packet_rate,
                          policer_conf.bit_rate_enable ? 1 /* KBPS */ : 0 /* PPS */);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/

static size_t acl_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[ACL_WEB_BUF_LEN];
    const char *sml_str = "function configHasAclPktRateGranularitySmallRange() {\n  return 0;\n}\n";

    if (ACL_MGMT_PACKET_RATE_GRANULARITY_SMALL_RANGE) {
        sml_str = "";
    }

    (void) snprintf(buff, ACL_WEB_BUF_LEN,
                    "var configPolicyMax = %d;\n"
                    "var configPolicyBitmaskMax = %d;\n"
                    "var configAclRateLimitIdMax = %d;\n"
                    "var configAceMax = %d;\n"
                    "var configAclPktRateMax = %d;\n"
                    "var configAclBitRateMax = %d;\n"
                    "var configAclBitRateGranularity = %d;\n"
                    "var configAclPktRateGranularity = %d;\n"
#if !defined(ACL_IPV6_SUPPORTED)
                    "function confighasAclIpv6() {\n"
                    "  return 0;\n"
                    "}\n"
#endif /* ACL_IPV6_SUPPORTED */
                    "%s"
                    , ACL_MGMT_POLICY_NO_MAX
                    , ACL_MGMT_POLICIES_BITMASK
                    , ACL_MGMT_RATE_LIMITER_NO_END
                    , ACL_MGMT_ACE_MAX
                    , ACL_MGMT_PACKET_RATE_MAX
                    , ACL_MGMT_BIT_RATE_MAX
                    , ACL_MGMT_BIT_RATE_GRANULARITY
                    , ACL_MGMT_PACKET_RATE_GRANULARITY == 1 ? 0 : ACL_MGMT_PACKET_RATE_GRANULARITY
                    , sml_str);
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  JS lib config table entry                                               */
/****************************************************************************/

web_lib_config_js_tab_entry(acl_lib_config_js);

/****************************************************************************/
/*  Module Filter CSS routine                                               */
/****************************************************************************/
static size_t acl_lib_filter_css(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[ACL_WEB_BUF_LEN];
    const char *rate_str;
    const char *sml_str = ".acl_pkt_granularity_small_range_only { display: none; }\r\n";

    if (ACL_MGMT_PACKET_RATE_GRANULARITY == 1) {
        rate_str = ".acl_pkt_rate_not_in_range_only { display: none; }\r\n";
    } else {
        rate_str = ".acl_pkt_rate_in_range_only { display: none; }\r\n";
    }
    if (ACL_MGMT_PACKET_RATE_GRANULARITY_SMALL_RANGE) {
        sml_str = "";
    }

    (void) snprintf(buff, ACL_WEB_BUF_LEN,
#if !defined(ACL_IPV6_SUPPORTED)
                    ".acl_ipv6_only { display: none; }\r\n"
#endif /* ACL_IPV6_SUPPORTED */
                    ".acl_v1_only { display: none; }\r\n"
                    "%s%s", rate_str, sml_str);
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  Filter CSS table entry                                                  */
/****************************************************************************/

web_lib_filter_css_tab_entry(acl_lib_filter_css);

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_acl, "/config/acl", handler_config_acl);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_acl_edit, "/config/acl_edit", handler_config_acl_edit);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_aclports, "/config/acl_ports", handler_config_acl_ports);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_acl, "/stat/acl", handler_status_acl);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_acl_ratelimiter, "/config/acl_ratelimiter", handler_config_acl_ratelimiter);

