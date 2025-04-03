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

/*
******************************************************************************

    Include files

******************************************************************************
*/
#include "icfg_api.h"
#include "icli_porting_util.h"
#include "acl_api.h"
#include "acl_icfg.h"
#include "mgmt_api.h"   //mgmt_acl_type_txt()
#include "misc_api.h"   //uport2iport(), iport2uport(), misc_strncpyz()
#include "msg_api.h"    //msg_switch_exists()
#include "topo_api.h"   //topo_usid2isid(), topo_isid2usid()

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_ACL

#define ACL_SNPRINTF(buf, size, ...)                                            \
    if ((size) > strlen(buf)) {                                                 \
        (void)snprintf((buf) + strlen(buf), (size) - strlen(buf), __VA_ARGS__); \
    }

/*
******************************************************************************

    Constant and Macro definition

******************************************************************************
*/
#define ACL_ICFG_BUF_SIZE    80

/*
******************************************************************************

    Data structure type definition

******************************************************************************
*/

/*
******************************************************************************

    Static Function

******************************************************************************
*/
static void ACL_ICFG_ace_flag(acl_entry_conf_t *conf_p, char *ace_str_p, acl_flag_t flag, BOOL *first_flag_p, const char *flag_str_p)
{
    char str_buf[ACL_ICFG_BUF_SIZE];

    misc_strncpyz(str_buf, mgmt_acl_flag_txt(conf_p, flag, TRUE), ACL_ICFG_BUF_SIZE);
    if (strcmp(str_buf, "any")) {
        if (*first_flag_p) {
            *first_flag_p = FALSE;
            ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s", flag_str_p);
        }

        switch (flag) {
        case ACE_FLAG_ARP_REQ:
            ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s %s", ACL_ARP_FLAG_REQUEST_TEXT, str_buf);
            break;
        case ACE_FLAG_ARP_SMAC:
            ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s %s", ACL_ARP_FLAG_SMAC_TEXT, str_buf);
            break;
        case ACE_FLAG_ARP_DMAC:
            ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s %s", ACL_ARP_FLAG_TMAC_TEXT, str_buf);
            break;
        case ACE_FLAG_ARP_LEN:
            ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s %s", ACL_ARP_FLAG_LEN_TEXT, str_buf);
            break;
        case ACE_FLAG_ARP_IP:
            ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s %s", ACL_ARP_FLAG_IP_TEXT, str_buf);
            break;
        case ACE_FLAG_ARP_ETHER:
            ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s %s", ACL_ARP_FLAG_ETHER_TEXT, str_buf);
            break;
        case ACE_FLAG_IP_TTL:
            ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s %s", ACL_IP_FLAG_TTL_TEXT, str_buf);
            break;
        case ACE_FLAG_IP_OPTIONS:
            ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s %s", ACL_IP_FLAG_OPT_TEXT, str_buf);
            break;
        case ACE_FLAG_IP_FRAGMENT:
            ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s %s", ACL_IP_FLAG_FRAG_TEXT, str_buf);
            break;
        case ACE_FLAG_TCP_FIN:
            ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s %s", ACL_TCP_FLAG_FIN_TEXT, str_buf);
            break;
        case ACE_FLAG_TCP_SYN:
            ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s %s", ACL_TCP_FLAG_SYN_TEXT, str_buf);
            break;
        case ACE_FLAG_TCP_RST:
            ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s %s", ACL_TCP_FLAG_RST_TEXT, str_buf);
            break;
        case ACE_FLAG_TCP_PSH:
            ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s %s", ACL_TCP_FLAG_PSH_TEXT, str_buf);
            break;
        case ACE_FLAG_TCP_ACK:
            ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s %s", ACL_TCP_FLAG_ACK_TEXT, str_buf);
            break;
        case ACE_FLAG_TCP_URG:
            ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s %s", ACL_TCP_FLAG_URG_TEXT, str_buf);
            break;
        default:
            break;
        }
    }
}

static void ACL_ICFG_ace_ipv6_tcp_flag(acl_entry_conf_t *conf_p, char *ace_str_p, acl_flag_t flag, BOOL *first_flag_p)
{
    if (conf_p->frame.ipv6.tcp_fin != MESA_ACE_BIT_ANY ||
        conf_p->frame.ipv6.tcp_syn != MESA_ACE_BIT_ANY ||
        conf_p->frame.ipv6.tcp_rst != MESA_ACE_BIT_ANY ||
        conf_p->frame.ipv6.tcp_psh != MESA_ACE_BIT_ANY ||
        conf_p->frame.ipv6.tcp_ack != MESA_ACE_BIT_ANY ||
        conf_p->frame.ipv6.tcp_urg != MESA_ACE_BIT_ANY) {
        if (*first_flag_p) {
            *first_flag_p = FALSE;
            ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s", ACL_TCP_FLAG_TEXT);
        }

        switch (flag) {
        case ACE_FLAG_TCP_FIN:
            if (conf_p->frame.ipv6.tcp_fin != MESA_ACE_BIT_ANY) {
                ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s %s", ACL_TCP_FLAG_FIN_TEXT, conf_p->frame.ipv6.tcp_fin == MESA_ACE_BIT_0 ? "0" : "1");
            }
            break;
        case ACE_FLAG_TCP_SYN:
            if (conf_p->frame.ipv6.tcp_syn != MESA_ACE_BIT_ANY) {
                ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s %s", ACL_TCP_FLAG_SYN_TEXT, conf_p->frame.ipv6.tcp_syn == MESA_ACE_BIT_0 ? "0" : "1");
            }
            break;
        case ACE_FLAG_TCP_RST:
            if (conf_p->frame.ipv6.tcp_rst != MESA_ACE_BIT_ANY) {
                ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s %s", ACL_TCP_FLAG_RST_TEXT, conf_p->frame.ipv6.tcp_rst == MESA_ACE_BIT_0 ? "0" : "1");
            }
            break;
        case ACE_FLAG_TCP_PSH:
            if (conf_p->frame.ipv6.tcp_psh != MESA_ACE_BIT_ANY) {
                ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s %s", ACL_TCP_FLAG_PSH_TEXT, conf_p->frame.ipv6.tcp_psh == MESA_ACE_BIT_0 ? "0" : "1");
            }
            break;
        case ACE_FLAG_TCP_ACK:
            if (conf_p->frame.ipv6.tcp_ack != MESA_ACE_BIT_ANY) {
                ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s %s", ACL_TCP_FLAG_ACK_TEXT, conf_p->frame.ipv6.tcp_ack == MESA_ACE_BIT_0 ? "0" : "1");
            }
            break;
        case ACE_FLAG_TCP_URG:
            if (conf_p->frame.ipv6.tcp_urg != MESA_ACE_BIT_ANY) {
                ACL_SNPRINTF(ace_str_p, ICLI_STR_MAX_LEN, " %s %s", ACL_TCP_FLAG_URG_TEXT, conf_p->frame.ipv6.tcp_urg == MESA_ACE_BIT_0 ? "0" : "1");
            }
            break;
        default:
            break;
        }
    }
}

/* ICFG callback functions */
static mesa_rc ACL_ICFG_global_conf(
    const vtss_icfg_query_request_t     *req,
    vtss_icfg_query_result_t            *result
)
{
    mesa_rc                                 rc = VTSS_RC_OK;
    mesa_acl_policer_no_t                   policer_idx;
    vtss_appl_acl_config_rate_limiter_t     policer_conf;
    vtss_appl_acl_config_rate_limiter_t     def_policer_conf;
    int                                     policer_conf_changed = 0;
    int                                     policer_conf_all_the_same = TRUE;
    char                                    *ace_str = (char *)VTSS_MALLOC(ICLI_STR_MAX_LEN); /* Using dynamic memory to prevent stack size overflow */
    char                                    str_buf[ACL_ICFG_BUF_SIZE];
    mesa_ace_id_t                           *ace_id; /* Using dynamic memory to prevent stack size overflow */
    int                                     ace_id_idx = 0;
    int                                     ace_cnt = 0;
    acl_entry_conf_t                        ace_conf;
    acl_entry_conf_t                        default_ace_conf;
    BOOL                                    first_flag = TRUE;
#if defined(ACL_IPV6_SUPPORTED)
    u32                                     sip_v6_mask;
#endif /* ACL_IPV6_SUPPORTED */
    mesa_port_no_t                          port_idx;
    u32                                     port_list_cnt;
    u32                                     port_filter_list_cnt;
    u32                                     port_count = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    if (ace_str == NULL) {
        return VTSS_RC_ERROR;
    } else if ((ace_id = (mesa_ace_id_t *)VTSS_MALLOC(sizeof(mesa_ace_id_t) * (ACL_MGMT_ACE_ID_END + 1))) == NULL) {
        VTSS_FREE(ace_str);
        return VTSS_RC_ERROR;
    }

    //check if each sub-configuration is the same
    if ((rc = acl_mgmt_policer_conf_get(ACL_MGMT_RATE_LIMITER_NO_START, &def_policer_conf)) != VTSS_RC_OK) {
        VTSS_FREE(ace_str);
        VTSS_FREE(ace_id);
        return rc;
    }

    for (policer_idx = ACL_MGMT_RATE_LIMITER_NO_START + 1; policer_idx < ACL_MGMT_RATE_LIMITER_NO_END; policer_idx++) {
        if ((rc = acl_mgmt_policer_conf_get(policer_idx, &policer_conf)) != VTSS_RC_OK) {
            VTSS_FREE(ace_str);
            VTSS_FREE(ace_id);
            return rc;
        }
        if (acl_mgmt_policer_conf_changed(&def_policer_conf, &policer_conf)) {
            policer_conf_all_the_same = FALSE;
            break;  // found one is different
        }
    }

    for (policer_idx = ACL_MGMT_RATE_LIMITER_NO_START; policer_idx < ACL_MGMT_RATE_LIMITER_NO_END; policer_idx++) {
        if ((rc = acl_mgmt_policer_conf_get(policer_idx, &policer_conf)) != VTSS_RC_OK) {
            VTSS_FREE(ace_str);
            VTSS_FREE(ace_id);
            return rc;
        }

        acl_mgmt_policer_conf_get_default(&def_policer_conf);
        policer_conf_changed = acl_mgmt_policer_conf_changed(&def_policer_conf, &policer_conf);

        str_buf[0] = '\0';
        if (!policer_conf_all_the_same) {
            ACL_SNPRINTF(str_buf, ACL_ICFG_BUF_SIZE, " %u", ipolicer2upolicer(policer_idx));
        }

        //rate-limiter
        if (req->all_defaults || policer_conf_changed) {
            const char *txt;
            u32 rate;

            if (policer_conf.bit_rate_enable) {
                rate = (policer_conf.bit_rate / ACL_MGMT_BIT_RATE_GRANULARITY);
                txt = (ACL_MGMT_BIT_RATE_GRANULARITY == 25 ? "25kbps" : "100kbps");
            } else {
                rate = policer_conf.packet_rate;
                if (rate >= ACL_MGMT_PACKET_RATE_GRANULARITY) {
                    rate /= ACL_MGMT_PACKET_RATE_GRANULARITY;
                }
                txt = (ACL_MGMT_PACKET_RATE_GRANULARITY == 10 ? "10pps" : (ACL_MGMT_PACKET_RATE_GRANULARITY == 100 && (ACL_MGMT_PACKET_RATE_GRANULARITY_SMALL_RANGE ? rate >= ACL_MGMT_PACKET_RATE_GRANULARITY : TRUE)) ? "100pps" : "pps");
            }
            rc = vtss_icfg_printf(result, "%s %s%s %s %u\n",
                                  ACL_ACCESS_LIST_TEXT,
                                  ACL_RATE_LIMITER_TEXT,
                                  str_buf,
                                  txt, rate);
        }

        // do one time while all configuration is the same
        if (policer_conf_all_the_same) {
            break;
        }
    }

    /* We need to show the ACEs from the bottom to top.
       That the sytem won't occur an error of "cannot find the next ACE ID" when running the "startup-config". */
    ace_id[ace_id_idx] = ACL_MGMT_ACE_ID_NONE;
    while (acl_mgmt_ace_get(ACL_USER_STATIC, ace_id[ace_id_idx], &ace_conf, NULL, TRUE) == VTSS_RC_OK) {
        ace_id[ace_id_idx] = ace_id[ace_id_idx + 1] = ace_conf.id;
        ace_id_idx++;
        ace_cnt++;
    }
    if (ace_cnt == 0) {
        VTSS_FREE(ace_str);
        VTSS_FREE(ace_id);
        return rc;
    }

    for (ace_id_idx = ace_cnt - 1; ace_id_idx >= 0; ace_id_idx--) {
        if (acl_mgmt_ace_get(ACL_USER_STATIC, ace_id[ace_id_idx], &ace_conf, NULL, FALSE) != VTSS_RC_OK) {
            break;
        }

        ace_str[0] = '\0';
        ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, "%s %d", ACL_ACE_TEXT, ace_id[ace_id_idx]);

        //next_ace_id
        if (ace_id_idx != ace_cnt - 1) {
            ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " next %u", ace_id[ace_id_idx + 1]);
        }

        //ingress
        port_list_cnt = 0;
        port_filter_list_cnt = 0;
        for (port_idx = 0; port_idx < port_count; port_idx++) {
            if (ace_conf.port_list[port_idx]) {
                port_list_cnt++;
            }
            if (ace_conf.action.port_list[port_idx]) {
                port_filter_list_cnt++;
            }
        }

        if (port_list_cnt != port_count) { //specific switch port No.
            ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s %s",
                         ACL_INGRESS_TEXT,
                         ACL_INTERFACE_TEXT,
                         icli_port_list_info_txt(VTSS_ISID_START, ace_conf.port_list, str_buf, FALSE));
        }

        //policy
        if (ace_conf.policy.mask) {
            ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %u",
                         ACL_POLICY_TEXT,
                         ace_conf.policy.value);

            //policy-bitmask
            if (ace_conf.policy.mask != ACL_MGMT_POLICIES_BITMASK) {
                ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s 0x%X",
                             ACL_POLICY_BITMASK_TEXT,
                             ace_conf.policy.mask);
            }
        }

        //tag
        if (ace_conf.tagged != MESA_ACE_BIT_ANY) {
            ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s",
                         ACL_TAG_TEXT,
                         ace_conf.tagged == MESA_ACE_BIT_0 ? "untagged" : "tagged");
        }

        //vid
        if (ace_conf.vid.mask) {
            ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %u",
                         ACL_VID_TEXT,
                         ace_conf.vid.value);
        }

        //tag_priority
        if (ace_conf.usr_prio.mask) {
            u8 usr_prio_min, usr_prio_max;
            if (ace_conf.usr_prio.mask == 0x7) {
                ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %u",
                             ACL_TAG_PRIORITY_TEXT,
                             ace_conf.usr_prio.value);
            } else {
                if (ace_conf.usr_prio.mask == 0x6) {
                    if (ace_conf.usr_prio.value == 0) { // 0-1
                        usr_prio_min = 0;
                        usr_prio_max = 1;
                    } else if (ace_conf.usr_prio.value == 2) { // 2-3
                        usr_prio_min = 2;
                        usr_prio_max = 3;
                    } else if (ace_conf.usr_prio.value == 4) { // 4-5
                        usr_prio_min = 4;
                        usr_prio_max = 5;
                    } else { // 6-7
                        usr_prio_min = 6;
                        usr_prio_max = 7;
                    }
                } else {
                    if (ace_conf.usr_prio.value == 0) { // 0-3
                        usr_prio_min = 0;
                        usr_prio_max = 3;
                    } else { // 4-7
                        usr_prio_min = 4;
                        usr_prio_max = 7;
                    }
                }

                ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %u-%u",
                             ACL_TAG_PRIORITY_TEXT,
                             usr_prio_min,
                             usr_prio_max);
            }
        }

        //dmac_type
        if (mgmt_acl_dmac_txt(&ace_conf, str_buf, TRUE) &&
            strcmp(str_buf, "any")) {
            ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s",
                         ACL_DMAC_TYPE_TEXT,
                         str_buf);
        }

        //frametype
        if (acl_mgmt_ace_init(ace_conf.type, &default_ace_conf) != VTSS_RC_OK) {
            continue;
        }
        switch (ace_conf.type) {
        case MESA_ACE_TYPE_ETYPE:
            ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s", ACL_FRAMETYPE_TEXT, ACL_ETYPE_TEXT);
            if (memcmp(&ace_conf.frame, &default_ace_conf.frame, sizeof(ace_conf.frame.etype)) ||
                memcmp(&ace_conf.flags, &default_ace_conf.flags, sizeof(ace_conf.flags))) {
                //etype
                if (mgmt_acl_uchar2_txt(&ace_conf.frame.etype.etype, str_buf, TRUE) &&
                    strcmp(str_buf, "any")) {
                    ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s", ACL_ETYPE_VALUE_TEXT, str_buf);
                }

                //smac
                if (mgmt_acl_uchar6_txt(&ace_conf.frame.etype.smac, str_buf, TRUE) &&
                    strcmp(str_buf, "any")) {
                    ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s", ACL_SMAC_TEXT, str_buf);
                }

                //dmac
                if (mgmt_acl_uchar6_txt(&ace_conf.frame.etype.dmac, str_buf, TRUE) &&
                    strcmp(str_buf, "any")) {
                    ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s", ACL_DMAC_TEXT, str_buf);
                }
            }
            break;
        case MESA_ACE_TYPE_ARP:
            ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s", ACL_FRAMETYPE_TEXT, ACL_ARP_TEXT);
            if (memcmp(&ace_conf.frame, &default_ace_conf.frame, sizeof(ace_conf.frame.arp)) ||
                memcmp(&ace_conf.flags, &default_ace_conf.flags, sizeof(ace_conf.flags))) {
                //sip
                if (mgmt_acl_ipv4_txt(&ace_conf.frame.arp.sip, str_buf, TRUE) &&
                    strcmp(str_buf, "any")) {
                    ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s",
                                 ACL_SIP_TEXT,
                                 str_buf);
                }

                //dip
                if (mgmt_acl_ipv4_txt(&ace_conf.frame.arp.dip, str_buf, TRUE) &&
                    strcmp(str_buf, "any")) {
                    ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s",
                                 ACL_DIP_TEXT,
                                 str_buf);
                }

                //smac
                if (mgmt_acl_uchar6_txt(&ace_conf.frame.arp.smac, str_buf, TRUE) &&
                    strcmp(str_buf, "any")) {
                    ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s",
                                 ACL_SMAC_TEXT,
                                 str_buf);
                }

                //opcode
                if (mgmt_acl_opcode_txt(&ace_conf, str_buf, TRUE) &&
                    strcmp(str_buf, "any")) {
                    ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s",
                                 ACL_ARP_OPCODE_TEXT,
                                 str_buf);
                }

                //arp_flag
                first_flag = TRUE;
                ACL_ICFG_ace_flag(&ace_conf, ace_str, ACE_FLAG_ARP_REQ, &first_flag, "arp-flag");
                ACL_ICFG_ace_flag(&ace_conf, ace_str, ACE_FLAG_ARP_SMAC, &first_flag, "arp-flag");
                ACL_ICFG_ace_flag(&ace_conf, ace_str, ACE_FLAG_ARP_DMAC, &first_flag, "arp-flag");
                ACL_ICFG_ace_flag(&ace_conf, ace_str, ACE_FLAG_ARP_LEN, &first_flag, "arp-flag");
                ACL_ICFG_ace_flag(&ace_conf, ace_str, ACE_FLAG_ARP_IP, &first_flag, "arp-flag");
                ACL_ICFG_ace_flag(&ace_conf, ace_str, ACE_FLAG_ARP_ETHER, &first_flag, "arp-flag");
            }
            break;
        case MESA_ACE_TYPE_IPV4:
            if (memcmp(&ace_conf.frame, &default_ace_conf.frame, sizeof(ace_conf.frame.ipv4)) == 0 &&
                memcmp(&ace_conf.flags, &default_ace_conf.flags, sizeof(ace_conf.flags)) == 0) {
                ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s", ACL_FRAMETYPE_TEXT, ACL_IP_TEXT);
            } else {
                //ipv4_protocol
                switch (ace_conf.frame.ipv4.proto.value) {
                case 1: //icmp
                    ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s", ACL_FRAMETYPE_TEXT, ACL_ICMP_TEXT);
                    break;
                case 6: //tcp
                    ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s", ACL_FRAMETYPE_TEXT, ACL_TCP_TEXT);
                    break;
                case 17: //udp
                    ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s", ACL_FRAMETYPE_TEXT, ACL_UDP_TEXT);
                    break;
                default:
                    ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s",
                                 ACL_FRAMETYPE_TEXT,
                                 ACL_IP_TEXT);
                    if (ace_conf.frame.ipv4.proto.mask) {
                        ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %u",
                                     ACL_IP_PROTOCOL_TEXT,
                                     ace_conf.frame.ipv4.proto.value);
                    }
                    break;
                }

                //sip
                if (mgmt_acl_ipv4_txt(&ace_conf.frame.ipv4.sip, str_buf, TRUE) &&
                    strcmp(str_buf, "any")) {
                    ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s",
                                 ACL_SIP_TEXT,
                                 str_buf);
                }

                //dip
                if (mgmt_acl_ipv4_txt(&ace_conf.frame.ipv4.dip, str_buf, TRUE) &&
                    strcmp(str_buf, "any")) {
                    ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s",
                                 ACL_DIP_TEXT,
                                 str_buf);
                }

                if (ace_conf.frame.ipv4.proto.value == 1) {
                    //icmpv4_type
                    if (mgmt_acl_ulong_txt(ace_conf.frame.ipv4.data.value[0], ace_conf.frame.ipv4.data.mask[0], str_buf, TRUE) &&
                        strcmp(str_buf, "any")) {
                        ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s",
                                     ACL_ICMP_TYPE_TEXT,
                                     str_buf);
                    }

                    //icmpv4_code
                    if (mgmt_acl_ulong_txt(ace_conf.frame.ipv4.data.value[1], ace_conf.frame.ipv4.data.mask[1], str_buf, TRUE) &&
                        strcmp(str_buf, "any")) {
                        ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s",
                                     ACL_ICMP_CODE_TEXT,
                                     str_buf);
                    }
                } else if (ace_conf.frame.ipv4.proto.value == 6 ||
                           ace_conf.frame.ipv4.proto.value == 17) {
                    //sportv4/dportv4
                    if (mgmt_acl_port_txt(&ace_conf.frame.ipv4.sport, str_buf, TRUE) &&
                        strcmp(str_buf, "any")) {
                        ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %u",
                                     ACL_SPORT_TEXT,
                                     ace_conf.frame.ipv4.sport.low);
                        if (ace_conf.frame.ipv4.sport.low != ace_conf.frame.ipv4.sport.high) {
                            ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " to %u",
                                         ace_conf.frame.ipv4.sport.high);
                        }
                    }

                    if (mgmt_acl_port_txt(&ace_conf.frame.ipv4.dport, str_buf, TRUE) &&
                        strcmp(str_buf, "any")) {
                        ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %u",
                                     ACL_DPORT_TEXT,
                                     ace_conf.frame.ipv4.dport.low);
                        if (ace_conf.frame.ipv4.dport.low != ace_conf.frame.ipv4.dport.high) {
                            ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " to %u",
                                         ace_conf.frame.ipv4.dport.high);
                        }
                    }
                }

                //ip_flag
                first_flag = TRUE;
                ACL_ICFG_ace_flag(&ace_conf, ace_str, ACE_FLAG_IP_TTL, &first_flag, ACL_IP_FLAG_TEXT);
                ACL_ICFG_ace_flag(&ace_conf, ace_str, ACE_FLAG_IP_OPTIONS, &first_flag, ACL_IP_FLAG_TEXT);
                ACL_ICFG_ace_flag(&ace_conf, ace_str, ACE_FLAG_IP_FRAGMENT, &first_flag, ACL_IP_FLAG_TEXT);

                //tcpv4_flag
                if (ace_conf.frame.ipv4.proto.value == 6) {
                    first_flag = TRUE;
                    ACL_ICFG_ace_flag(&ace_conf, ace_str, ACE_FLAG_TCP_FIN, &first_flag, ACL_TCP_FLAG_TEXT);
                    ACL_ICFG_ace_flag(&ace_conf, ace_str, ACE_FLAG_TCP_SYN, &first_flag, ACL_TCP_FLAG_TEXT);
                    ACL_ICFG_ace_flag(&ace_conf, ace_str, ACE_FLAG_TCP_RST, &first_flag, ACL_TCP_FLAG_TEXT);
                    ACL_ICFG_ace_flag(&ace_conf, ace_str, ACE_FLAG_TCP_PSH, &first_flag, ACL_TCP_FLAG_TEXT);
                    ACL_ICFG_ace_flag(&ace_conf, ace_str, ACE_FLAG_TCP_ACK, &first_flag, ACL_TCP_FLAG_TEXT);
                    ACL_ICFG_ace_flag(&ace_conf, ace_str, ACE_FLAG_TCP_URG, &first_flag, ACL_TCP_FLAG_TEXT);
                }
            }
            break;
#if defined(ACL_IPV6_SUPPORTED)
        case MESA_ACE_TYPE_IPV6:
            if (memcmp(&ace_conf.frame, &default_ace_conf.frame, sizeof(ace_conf.frame.ipv6)) == 0 &&
                memcmp(&ace_conf.flags, &default_ace_conf.flags, sizeof(ace_conf.flags)) == 0) {
                ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s", ACL_FRAMETYPE_TEXT, ACL_IPV6_TEXT);
            } else {
                switch (ace_conf.frame.ipv6.proto.value) {
                case 58: //icmp
                    ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s", ACL_FRAMETYPE_TEXT, ACL_IPV6_ICMP_TEXT);
                    break;
                case 6: //tcp
                    ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s", ACL_FRAMETYPE_TEXT, ACL_IPV6_TCP_TEXT);
                    break;
                case 17: //udp
                    ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s", ACL_FRAMETYPE_TEXT, ACL_IPV6_UDP_TEXT);
                    break;
                default:
                    ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s",
                                 ACL_FRAMETYPE_TEXT,
                                 ACL_IPV6_TEXT);
                    if (ace_conf.frame.ipv6.proto.mask) {
                        ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %u",
                                     ACL_NEXT_HEADER_TEXT,
                                     ace_conf.frame.ipv6.proto.value);
                    }
                    break;
                }

                //sipv6_bitmask
                sip_v6_mask = (ace_conf.frame.ipv6.sip.mask[12] << 24) +
                              (ace_conf.frame.ipv6.sip.mask[13] << 16) +
                              (ace_conf.frame.ipv6.sip.mask[14] << 8) +
                              ace_conf.frame.ipv6.sip.mask[15];
                if (sip_v6_mask) {
                    //sipv6
                    if (mgmt_acl_ipv6_txt(&ace_conf.frame.ipv6.sip, str_buf, TRUE)) {
                        if (strcmp(str_buf, "any")) {
                            ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s",
                                         ACL_SIP_TEXT,
                                         str_buf);

                            if (sip_v6_mask != 0xFFFFFFFF) {
                                ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s 0x%X",
                                             ACL_SIP_BITMASK_TEXT,
                                             sip_v6_mask);
                            }
                        }
                    }
                }

                if (ace_conf.frame.ipv6.proto.value == 58) {
                    //icmpv6_type
                    if (mgmt_acl_ulong_txt(ace_conf.frame.ipv6.data.value[0], ace_conf.frame.ipv6.data.mask[0], str_buf, TRUE) &&
                        strcmp(str_buf, "any")) {
                        ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s",
                                     ACL_ICMP_TYPE_TEXT,
                                     str_buf);
                    }

                    //icmpv6_code
                    if (mgmt_acl_ulong_txt(ace_conf.frame.ipv6.data.value[1], ace_conf.frame.ipv6.data.mask[1], str_buf, TRUE) &&
                        strcmp(str_buf, "any")) {
                        ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s",
                                     ACL_ICMP_CODE_TEXT,
                                     str_buf);
                    }
                } else if (ace_conf.frame.ipv6.proto.value == 6 ||
                           ace_conf.frame.ipv6.proto.value == 17) {
                    //sportv6/dportv6
                    if (mgmt_acl_port_txt(&ace_conf.frame.ipv6.sport, str_buf, TRUE) &&
                        strcmp(str_buf, "any")) {
                        ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %u",
                                     ACL_SPORT_TEXT,
                                     ace_conf.frame.ipv6.sport.low);
                        if (ace_conf.frame.ipv6.sport.low != ace_conf.frame.ipv6.sport.high) {
                            ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " to %u",
                                         ace_conf.frame.ipv6.sport.high);
                        }
                    }

                    if (mgmt_acl_port_txt(&ace_conf.frame.ipv6.dport, str_buf, TRUE) &&
                        strcmp(str_buf, "any")) {
                        ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %u",
                                     ACL_DPORT_TEXT,
                                     ace_conf.frame.ipv6.dport.low);
                        if (ace_conf.frame.ipv6.dport.low != ace_conf.frame.ipv6.dport.high) {
                            ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " to %u",
                                         ace_conf.frame.ipv6.dport.high);
                        }
                    }
                }

                //hop_limit
                if (ace_conf.frame.ipv6.ttl != MESA_ACE_BIT_ANY) {
                    ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s",
                                 ACL_HOP_LIMIT_TEXT,
                                 ace_conf.frame.ipv6.ttl == MESA_ACE_BIT_0 ? "0" : "1");
                }

                //tcpv6_flag
                if (ace_conf.frame.ipv6.proto.value == 6) {
                    first_flag = TRUE;
                    ACL_ICFG_ace_ipv6_tcp_flag(&ace_conf, ace_str, ACE_FLAG_TCP_FIN, &first_flag);
                    ACL_ICFG_ace_ipv6_tcp_flag(&ace_conf, ace_str, ACE_FLAG_TCP_SYN, &first_flag);
                    ACL_ICFG_ace_ipv6_tcp_flag(&ace_conf, ace_str, ACE_FLAG_TCP_RST, &first_flag);
                    ACL_ICFG_ace_ipv6_tcp_flag(&ace_conf, ace_str, ACE_FLAG_TCP_PSH, &first_flag);
                    ACL_ICFG_ace_ipv6_tcp_flag(&ace_conf, ace_str, ACE_FLAG_TCP_ACK, &first_flag);
                    ACL_ICFG_ace_ipv6_tcp_flag(&ace_conf, ace_str, ACE_FLAG_TCP_URG, &first_flag);
                }
            }
            break;
#endif /* ACL_IPV6_SUPPORTED */
        default:
            break;
        }

        //action
        if (ace_conf.action.port_action != MESA_ACL_PORT_ACTION_NONE) {
            if (ace_conf.action.port_action == MESA_ACL_PORT_ACTION_FILTER && port_filter_list_cnt != 0) {
                ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s %s %s",
                             ACL_ACTION_TEXT,
                             ACL_FILTER_TEXT,
                             ACL_INTERFACE_TEXT,
                             icli_port_list_info_txt(VTSS_ISID_START, ace_conf.action.port_list, str_buf, FALSE));
            } else {
                ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s", ACL_ACTION_TEXT, ACL_DENY_TEXT);
            }
        }

        //rate_limter_id
        if (ace_conf.action.policer != ACL_MGMT_RATE_LIMITER_NONE) {
            ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %u",
                         ACL_RATE_LIMITER_TEXT,
                         ipolicer2upolicer(ace_conf.action.policer));
        }

        //mirror
        if (ace_conf.action.mirror) {
            ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s", ACL_MIRROR_TEXT);
        }

        //redirect
        if (ace_conf.action.port_action == MESA_ACL_PORT_ACTION_REDIR) {
            ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s %s %s",
                         ACL_REDIRECT_TEXT,
                         ACL_INTERFACE_TEXT,
                         icli_port_list_info_txt(VTSS_ISID_START, ace_conf.action.port_list, str_buf, FALSE));
        }

        //logging
        if (ace_conf.action.logging) {
            ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s", ACL_LOGGING_TEXT);
        }

        //shutdown
        if (ace_conf.action.shutdown) {
            ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s", ACL_SHUTDOWN_TEXT);
        }

        //lookup
        if (ace_conf.lookup) {
            ACL_SNPRINTF(ace_str, ICLI_STR_MAX_LEN, " %s", ACL_LOOKUP_TEXT);
        }

        if (vtss_icfg_printf(result, "%s\n", ace_str) != VTSS_RC_OK) {
            break;
        }
    }

    VTSS_FREE(ace_str);
    VTSS_FREE(ace_id);
    return rc;
}

static mesa_rc ACL_ICFG_port_conf(const vtss_icfg_query_request_t *req,
                                  vtss_icfg_query_result_t *result)
{
    mesa_rc         rc = VTSS_RC_OK;
    acl_port_conf_t conf, def_conf;
    vtss_isid_t     isid = topo_usid2isid(req->instance_id.port.usid);
    mesa_port_no_t  iport = uport2iport(req->instance_id.port.begin_uport);
    int             conf_changed = 0;
    char            str_buf[ACL_ICFG_BUF_SIZE];

    if (req->instance_id.port.port_type == ICLI_PORT_TYPE_CPU) {
        return rc;
    }

    if ((rc = acl_mgmt_port_conf_get(iport, &conf)) != VTSS_RC_OK) {
        return rc;
    }

    acl_mgmt_port_conf_get_default(&def_conf);
    conf_changed = acl_mgmt_port_conf_changed(&def_conf, &conf);

    if (req->all_defaults ||
        (conf_changed && conf.action.port_action != def_conf.action.port_action)) {
        rc += vtss_icfg_printf(result, " %s %s %s\n",
                               ACL_ACCESS_LIST_TEXT,
                               ACL_ACTION_TEXT,
                               conf.action.port_action == MESA_ACL_PORT_ACTION_NONE ? ACL_PERMIT_TEXT : ACL_DENY_TEXT);
    }

    //rate-limiter
    str_buf[0] = '\0';
    if (req->all_defaults ||
        (conf_changed && conf.action.policer != def_conf.action.policer)) {
        ACL_SNPRINTF(str_buf, ACL_ICFG_BUF_SIZE, "%u", ipolicer2upolicer(conf.action.policer));
        rc += vtss_icfg_printf(result, " %s%s %s %s\n",
                               conf.action.policer != MESA_ACL_POLICY_NO_NONE ? "" : ACL_NO_FORM_TEXT,
                               ACL_ACCESS_LIST_TEXT,
                               ACL_RATE_LIMITER_TEXT,
                               conf.action.policer == MESA_ACL_POLICY_NO_NONE ? "" : str_buf);
    }

    //policy
    if (req->all_defaults ||
        (conf_changed && conf.policy_no != def_conf.policy_no)) {
        rc += vtss_icfg_printf(result, " %s %s %u\n",
                               ACL_ACCESS_LIST_TEXT,
                               ACL_POLICY_TEXT,
                               conf.policy_no);
    }

    //redirect
    if (req->all_defaults ||
        (conf_changed && conf.action.port_action != def_conf.action.port_action)) {
        rc += vtss_icfg_printf(result, " %s%s %s%s%s%s%s\n",
                               conf.action.port_action != MESA_ACL_PORT_ACTION_REDIR ? ACL_NO_FORM_TEXT : "",
                               ACL_ACCESS_LIST_TEXT,
                               ACL_REDIRECT_TEXT,
                               conf.action.port_action != MESA_ACL_PORT_ACTION_REDIR ? "" : " ",
                               conf.action.port_action != MESA_ACL_PORT_ACTION_REDIR ? "" : ACL_INTERFACE_TEXT,
                               conf.action.port_action != MESA_ACL_PORT_ACTION_REDIR ? "" : " ",
                               conf.action.port_action != MESA_ACL_PORT_ACTION_REDIR ? "" : icli_port_list_info_txt(isid, conf.action.port_list, str_buf, FALSE));
    }

    //mirror
    if (req->all_defaults ||
        (conf_changed && conf.action.mirror != def_conf.action.mirror)) {
        rc += vtss_icfg_printf(result, " %s%s %s\n",
                               conf.action.mirror ? "" : ACL_NO_FORM_TEXT,
                               ACL_ACCESS_LIST_TEXT,
                               ACL_MIRROR_TEXT);
    }

    //logging
    if (req->all_defaults ||
        (conf_changed && conf.action.logging != def_conf.action.logging)) {
        rc += vtss_icfg_printf(result, " %s%s %s\n",
                               conf.action.logging ? "" : ACL_NO_FORM_TEXT,
                               ACL_ACCESS_LIST_TEXT,
                               ACL_LOGGING_TEXT);
    }

    //shutdown
    if (req->all_defaults ||
        (conf_changed && conf.action.shutdown != def_conf.action.shutdown)) {
        rc += vtss_icfg_printf(result, " %s%s %s\n",
                               conf.action.shutdown ? "" : ACL_NO_FORM_TEXT,
                               ACL_ACCESS_LIST_TEXT,
                               ACL_SHUTDOWN_TEXT);
    }
    return rc;
}

/*
******************************************************************************

    Public functions

******************************************************************************
*/

/* Initialization function */
mesa_rc acl_icfg_init(void)
{
    mesa_rc rc;

    /* Register callback functions to ICFG module.
       The configuration divided into two groups for this module.
       1. Global configuration
       2. Port configuration
    */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_ACL_GLOBAL_CONF, "access-list", ACL_ICFG_global_conf)) != VTSS_RC_OK) {
        return rc;
    }

    rc = vtss_icfg_query_register(VTSS_ICFG_ACL_PORT_CONF, "access-list", ACL_ICFG_port_conf);

    return rc;
}
