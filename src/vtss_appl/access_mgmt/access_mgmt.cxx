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

#include "main.h"
#include "msg_api.h"
#include "critd_api.h"
#include "misc_api.h"
#include "packet_api.h"
#include "access_mgmt_api.h"
#include "access_mgmt.h"

#include <netinet/in.h>
#include <netinet/ip.h>
#ifdef VTSS_SW_OPTION_IPV6
#include <netinet/ip6.h>
#endif
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include "ip_api.h"
#include "vlan_api.h"

#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#endif

#include "vtss_os_wrapper_network.h"

#ifdef VTSS_SW_OPTION_ICFG
#include "access_mgmt_icfg.h"
#endif

#define VTSS_ALLOC_MODULE_ID    VTSS_MODULE_ID_ACCESS_MGMT


/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/
/* Access management statistics */
static u32 ACCESS_MGMT_http_receive_cnt     = 0, ACCESS_MGMT_http_discard_cnt   = 0;
static u32 ACCESS_MGMT_https_receive_cnt    = 0, ACCESS_MGMT_https_discard_cnt  = 0;
static u32 ACCESS_MGMT_snmp_receive_cnt     = 0, ACCESS_MGMT_snmp_discard_cnt   = 0;
static u32 ACCESS_MGMT_telnet_receive_cnt   = 0, ACCESS_MGMT_telnet_discard_cnt = 0;
static u32 ACCESS_MGMT_ssh_receive_cnt      = 0, ACCESS_MGMT_ssh_discard_cnt    = 0;

/* Global structure */
static access_mgmt_global_t ACCESS_MGMT_global;

static vtss_trace_reg_t ACCESS_MGMT_trace_reg = {
    VTSS_TRACE_MODULE_ID, "accessmgmt", "access management"
};

static vtss_trace_grp_t ACCESS_MGMT_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&ACCESS_MGMT_trace_reg, ACCESS_MGMT_trace_grps);

#define ACCESS_MGMT_CRIT_ENTER() critd_enter(&ACCESS_MGMT_global.crit, __FILE__, __LINE__)
#define ACCESS_MGMT_CRIT_EXIT()  critd_exit(&ACCESS_MGMT_global.crit,  __FILE__, __LINE__)

/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/
/* Registration for SNMP access management callbacks */
typedef enum {
    IP_STACK_GLUE_APPL_TYPE_NONE,
    IP_STACK_GLUE_APPL_TYPE_ALL         = 0x1,
    IP_STACK_GLUE_APPL_TYPE_SSH         = 0X2,  /* TCP 22 */
    IP_STACK_GLUE_APPL_TYPE_TELNET      = 0x4,  /* TCP 23 */
    IP_STACK_GLUE_APPL_TYPE_HTTP        = 0x8,  /* TCP 80 */
    IP_STACK_GLUE_APPL_TYPE_SNMP        = 0x10, /* UDP 161 */
    IP_STACK_GLUE_APPL_TYPE_HTTPS       = 0x20, /* TCP 443 */
    IP_STACK_GLUE_APPL_TYPE_ACCESS_MGMT = IP_STACK_GLUE_APPL_TYPE_HTTP | IP_STACK_GLUE_APPL_TYPE_HTTPS | IP_STACK_GLUE_APPL_TYPE_SNMP | IP_STACK_GLUE_APPL_TYPE_TELNET | IP_STACK_GLUE_APPL_TYPE_SSH
} ip_stack_glue_appl_type_t;

typedef struct {
    unsigned short  eth_type;
    unsigned char   ip_version;
    unsigned char   ip_protocol;
    unsigned long   sip;
    unsigned long   dip;
    unsigned short  sport;
    unsigned short  dport;
#ifdef VTSS_SW_OPTION_IPV6
    unsigned char   sip_v6[16];
    unsigned char   dip_v6[16];
#endif /* VTSS_SW_OPTION_IPV6 */
    ip_stack_glue_appl_type_t appl_type;
} ip_stack_glue_frame_info_t;

/* access_mgmt service type text */
const char *access_mgmt_service_type_txt(ip_stack_glue_appl_type_t service_type)
{
    const char *txt;

    switch (service_type) {
    case IP_STACK_GLUE_APPL_TYPE_SSH:
        txt = "SSH";
        break;
    case IP_STACK_GLUE_APPL_TYPE_TELNET:
        txt = "TELNET";
        break;
    case IP_STACK_GLUE_APPL_TYPE_HTTP:
        txt = "HTTP";
        break;
    case IP_STACK_GLUE_APPL_TYPE_SNMP:
        txt = "SNMP";
        break;
    case IP_STACK_GLUE_APPL_TYPE_HTTPS:
        txt = "HTTPS";
        break;
    default:
        txt = "access_mgmt unknown service type";
        break;
    }
    return txt;
}

/* Set access_mgmt defaults */
static void ACCESS_MGMT_default_set(access_mgmt_conf_t *conf)
{
    memset(conf, 0x0, sizeof(*conf));
    conf->mode = ACCESS_MGMT_DISABLED;
}

/* Determine if access_mgmt configuration has changed */
static int ACCESS_MGMT_compare_cfg(access_mgmt_conf_t *old_conf, access_mgmt_conf_t *new_conf)
{
    return memcmp(new_conf, old_conf, sizeof(*new_conf));
}

static void ACCESS_MGMT_internal_db_reset(void)
{
    u32                                 idx;
    access_mgmt_inter_module_entry_t    *ety;

    ACCESS_MGMT_CRIT_ENTER();
    for (idx = 0; idx < ACCESS_MGMT_MAX_ENTRIES; ++idx) {
        ety = &ACCESS_MGMT_global.access_mgmt_internal[idx];
        memset(ety, 0x0, sizeof(access_mgmt_inter_module_entry_t));
        ety->source = ACCESS_MGMT_INTERNAL_TYPE_NONE;
    }
    ACCESS_MGMT_global.access_mgmt_internal_count = 0;
    ACCESS_MGMT_CRIT_EXIT();
}

static BOOL ACCESS_MGMT_ipa_valid(const mesa_ip_addr_t *const ipa)
{
    u8  idx;

    if (!ipa) {
        return FALSE;
    }

    /* Access Management address entry should always be either active IPv4 or IPv6. */
    switch ( ipa->type ) {
    case MESA_IP_TYPE_IPV4:
        if (ipa->addr.ipv4 == 0) {
            return FALSE;
        }

        idx = (u8)((ipa->addr.ipv4 >> 24) & 0xFF);
        if ((idx == 127) ||
            ((idx > 223) && (idx < 240))) {
            return FALSE;
        }

        break;
    case MESA_IP_TYPE_IPV6:
        idx = ipa->addr.ipv6.addr[0];
        if (idx == 0xFF) {
            return FALSE;
        }

        for (idx = 0; idx < 16; idx++) {
            if (ipa->addr.ipv6.addr[idx]) {
                break;
            } else {
                if ((idx == 15) &&
                    ((ipa->addr.ipv6.addr[idx] == 0) || (ipa->addr.ipv6.addr[idx] == 1))) {
                    return FALSE;
                }
            }
        }

        break;
    case MESA_IP_TYPE_NONE:
    default:
        return FALSE;
    }

    return TRUE;
}

/* Callback function for IP stack glue layer
   Return 1: not allowed to hit TCP/IP stack
   Return 0: allowed to hit TCP/IP stack */
static BOOL ACCESS_MGMT_ip_stack_glue_callback(mesa_vid_t vid, ip_stack_glue_frame_info_t *frame_info)
{
    int access_id, service_type;
    access_mgmt_conf_t  conf;
#ifdef VTSS_SW_OPTION_SYSLOG
    char buf[40];
#ifdef VTSS_SW_OPTION_IPV6
    mesa_ipv6_t sip_v6;
#endif
#endif /* VTSS_SW_OPTION_SYSLOG */

#ifdef VTSS_SW_OPTION_IPV6
    if (frame_info->eth_type != 0x86DD && frame_info->eth_type != 0x0800) {
        return 0;
    }
#else
    if (frame_info->ip_version != 4 || frame_info->eth_type != 0x0800) {
        return 0;
    }
#endif /* VTSS_SW_OPTION_IPV6 */

    /* Check incoming frame is WEB, SNMP or Telnet */
    ACCESS_MGMT_CRIT_ENTER();
    if (frame_info->appl_type == IP_STACK_GLUE_APPL_TYPE_TELNET) {          /* TCP 23 - Telnet */
        service_type = ACCESS_MGMT_SERVICES_TYPE_TELNET;
        ACCESS_MGMT_telnet_receive_cnt++;
#ifdef VTSS_SW_OPTION_SSH
    } else if (frame_info->appl_type == IP_STACK_GLUE_APPL_TYPE_SSH) {      /* TCP 22 - SSH */
        service_type = ACCESS_MGMT_SERVICES_TYPE_TELNET;
        ACCESS_MGMT_ssh_receive_cnt++;
#endif /* VTSS_SW_OPTION_SSH */
    } else if (frame_info->appl_type == IP_STACK_GLUE_APPL_TYPE_HTTP) {     /* TCP 80 - WEB */
        service_type = ACCESS_MGMT_SERVICES_TYPE_WEB;
        ACCESS_MGMT_http_receive_cnt++;
#if defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
    } else if (frame_info->appl_type == IP_STACK_GLUE_APPL_TYPE_HTTPS) {    /* TCP 443 - HTTPS */
        service_type = ACCESS_MGMT_SERVICES_TYPE_WEB;
        ACCESS_MGMT_https_receive_cnt++;
#endif /* VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI */
    } else if (frame_info->appl_type == IP_STACK_GLUE_APPL_TYPE_SNMP) {   /* UDP 161 - SNMP */
        service_type = ACCESS_MGMT_SERVICES_TYPE_SNMP;
        ACCESS_MGMT_snmp_receive_cnt++;
    } else {
        ACCESS_MGMT_CRIT_EXIT();
        return 0;
    }
    ACCESS_MGMT_CRIT_EXIT();

    if (access_mgmt_conf_get(&conf) != VTSS_RC_OK) {
        return 0;
    }

    if (conf.mode == 0) {
        return 0;
    }

    for (access_id = ACCESS_MGMT_ACCESS_ID_START; access_id < ACCESS_MGMT_ACCESS_ID_START + ACCESS_MGMT_MAX_ENTRIES; access_id++) {
        if (conf.entry[access_id].valid
            && (service_type & conf.entry[access_id].service_type)
            && conf.entry[access_id].vid == vid
           ) {
            if (frame_info->ip_version == 4) {
                if (conf.entry[access_id].entry_type == ACCESS_MGMT_ENTRY_TYPE_IPV6) {
                    continue;
                }
                if ((frame_info->sip >= conf.entry[access_id].start_ip) && (frame_info->sip <= conf.entry[access_id].end_ip)) {
                    return 0;
                }
#ifndef VTSS_SW_OPTION_IPV6
            }
#else
            } else {
                if (conf.entry[access_id].entry_type == ACCESS_MGMT_ENTRY_TYPE_IPV4) {
                    continue;
                }
                if (memcmp(frame_info->sip_v6, &conf.entry[access_id].start_ipv6, sizeof(mesa_ipv6_t)) >= 0 && memcmp(frame_info->sip_v6, &conf.entry[access_id].end_ipv6, sizeof(mesa_ipv6_t)) <= 0) {
                    return 0;
                }
            }
#endif /* VTSS_SW_OPTION_IPV6 */
        }
    }

    /* Counter drop frame */
    ACCESS_MGMT_CRIT_ENTER();

    if (frame_info->appl_type == IP_STACK_GLUE_APPL_TYPE_TELNET)            /* TCP 23 - Telnet */
    {
        ACCESS_MGMT_telnet_discard_cnt++;
#ifdef VTSS_SW_OPTION_SSH
    } else if (frame_info->appl_type == IP_STACK_GLUE_APPL_TYPE_SSH)        /* TCP 22 - SSH */
    {
        ACCESS_MGMT_ssh_discard_cnt++;
#endif /* VTSS_SW_OPTION_SSH */
    } else if (frame_info->appl_type == IP_STACK_GLUE_APPL_TYPE_HTTP)       /* TCP 80 - WEB */
    {
        ACCESS_MGMT_http_discard_cnt++;
#if defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
    } else if (frame_info->appl_type == IP_STACK_GLUE_APPL_TYPE_HTTPS)      /* TCP 443 - HTTPS */
    {
        ACCESS_MGMT_https_discard_cnt++;
#endif /* VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI */
    } else if (frame_info->appl_type == IP_STACK_GLUE_APPL_TYPE_SNMP)       /* UDP 161 - SNMP */
    {
        ACCESS_MGMT_snmp_discard_cnt++;
    }

    ACCESS_MGMT_CRIT_EXIT();

#ifdef VTSS_SW_OPTION_SYSLOG
#ifdef VTSS_SW_OPTION_IPV6
    if (frame_info->ip_version == 4)
    {
        S_I("ACCESS_MGMT-ACCESS_DENIED: Access management filter reject %s access from IP address %s.", access_mgmt_service_type_txt(frame_info->appl_type), misc_ipv4_txt(frame_info->sip, buf));
    } else
    {
        memcpy(sip_v6.addr, frame_info->sip_v6, 16);
        S_I("ACCESS_MGMT-ACCESS_DENIED: Access management filter reject %s access from IP address %s.", access_mgmt_service_type_txt(frame_info->appl_type), misc_ipv6_txt(&sip_v6, buf));
    }
#else
    S_I("ACCESS_MGMT-ACCESS_DENIED: Access management filter reject %s access from IP address %s.",
        access_mgmt_service_type_txt(frame_info->appl_type), misc_ipv4_txt(frame_info->sip, buf));
#endif /* VTSS_SW_OPTION_IPV6 */
#endif /* VTSS_SW_OPTION_SYSLOG */

    return 1;
}

/* Access Management Thread */
static vtss_handle_t ACCESS_MGMT_thread_handle;
static vtss_thread_t ACCESS_MGMT_thread_block;
static vtss_flag_t   ACCESS_MGMT_thread_flag;

static BOOL          ACCESS_MGMT_state;

#define ACCESS_MGMT_EVENT_ANY                           0xFFFFFFFF  /* Any possible bit... */
#define ACCESS_MGMT_EVENT_POKE                          0x00000001
#define ACCESS_MGMT_EVENT_SLOW                          0x00000010
#define ACCESS_MGMT_EVENT_EXIT                          0x10000000

#define ACCESS_MGMT_IPTABLES_SP_ACCEPT(cmd, v, w, x, y, z) do { \
if ((z)) {                                                      \
  memset((cmd), 0x0, sizeof((cmd)));                            \
  if ((v) == 6) {                                               \
    if ((w)) {                                                  \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u %s -p tcp -m multiport --sports %u,%u -j ACCEPT", (w), (x), (y), (z)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u -p tcp -m multiport --sports %u,%u -j ACCEPT", (w), (y), (z)); \
    } else {                                                    \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT %s -p tcp -m multiport --sports %u,%u -j ACCEPT", (x), (y), (z)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -p tcp -m multiport --sports %u,%u -j ACCEPT", (y), (z)); \
    }                                                           \
  } else {                                                      \
    if ((w)) {                                                  \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u %s ! -f -p tcp -m multiport --sports %u,%u -j ACCEPT", (w), (x), (y), (z)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u ! -f -p tcp -m multiport --sports %u,%u -j ACCEPT", (w), (y), (z)); \
    } else {                                                    \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT %s ! -f -p tcp -m multiport --sports %u,%u -j ACCEPT", (x), (y), (z)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT ! -f -p tcp -m multiport --sports %u,%u -j ACCEPT", (y), (z)); \
    }                                                           \
  }                                                             \
  system((cmd));                                                \
  memset((cmd), 0x0, sizeof((cmd)));                            \
  if ((v) == 6) {                                               \
    if ((w)) {                                                  \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u %s -p udp -m multiport --sports %u,%u -j ACCEPT", (w), (x), (y), (z)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u -p udp -m multiport --sports %u,%u -j ACCEPT", (w), (y), (z)); \
    } else {                                                    \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT %s -p udp -m multiport --sports %u,%u -j ACCEPT", (x), (y), (z)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -p udp -m multiport --sports %u,%u -j ACCEPT", (y), (z)); \
    }                                                           \
  } else {                                                      \
    if ((w)) {                                                  \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u %s ! -f -p udp -m multiport --sports %u,%u -j ACCEPT", (w), (x), (y), (z)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u ! -f -p udp -m multiport --sports %u,%u -j ACCEPT", (w), (y), (z)); \
    } else {                                                    \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT %s ! -f -p udp -m multiport --sports %u,%u -j ACCEPT", (x), (y), (z)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT ! -f -p udp -m multiport --sports %u,%u -j ACCEPT", (y), (z)); \
    }                                                           \
  }                                                             \
  system((cmd));                                                \
} else {                                                        \
  memset((cmd), 0x0, sizeof((cmd)));                            \
  if ((v) == 6) {                                               \
    if ((w)) {                                                  \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u %s -p tcp -m tcp --dport %u -j ACCEPT", (w), (x), (y)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u -p tcp -m tcp --dport %u -j ACCEPT", (w), (y)); \
    } else {                                                    \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT %s -p tcp -m tcp --dport %u -j ACCEPT", (x), (y)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -p tcp -m tcp --dport %u -j ACCEPT", (y)); \
    }                                                           \
  } else {                                                      \
    if ((w)) {                                                  \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u %s ! -f -p tcp -m tcp --dport %u -j ACCEPT", (w), (x), (y)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u ! -f -p tcp -m tcp --dport %u -j ACCEPT", (w), (y)); \
    } else {                                                    \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT %s ! -f -p tcp -m tcp --dport %u -j ACCEPT", (x), (y)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT ! -f -p tcp -m tcp --dport %u -j ACCEPT", (y)); \
    }                                                           \
  }                                                             \
  system((cmd));                                                \
  memset((cmd), 0x0, sizeof((cmd)));                            \
  if ((v) == 6) {                                               \
    if ((w)) {                                                  \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u %s -p udp -m udp --dport %u -j ACCEPT", (w), (x), (y)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u -p udp -m udp --dport %u -j ACCEPT", (w), (y)); \
    } else {                                                    \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT %s -p udp -m udp --dport %u -j ACCEPT", (x), (y)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -p udp -m udp --dport %u -j ACCEPT", (y)); \
    }                                                           \
  } else {                                                      \
    if ((w)) {                                                  \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u %s ! -f -p udp -m udp --dport %u -j ACCEPT", (w), (x), (y)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u ! -f -p udp -m udp --dport %u -j ACCEPT", (w), (y)); \
    } else {                                                    \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT %s ! -f -p udp -m udp --dport %u -j ACCEPT", (x), (y)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT ! -f -p udp -m udp --dport %u -j ACCEPT", (y)); \
    }                                                           \
  }                                                             \
  system((cmd));                                                \
}                                                               \
} while (0)

#define ACCESS_MGMT_IPTABLES_SP_DROP(cmd, v, x, y, z)  do {     \
if ((z)) {                                                      \
  memset((cmd), 0x0, sizeof((cmd)));                            \
  if ((v) == 6) {                                               \
    if ((x)) {                                                  \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u -p tcp -m multiport --sports %u,%u -j DROP", (x), (y), (z)); \
    } else {                                                    \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -p tcp -m multiport --sports %u,%u -j DROP", (y), (z)); \
    }                                                           \
  } else {                                                      \
    if ((x)) {                                                  \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u ! -f -p tcp -m multiport --sports %u,%u -j DROP", (x), (y), (z)); \
    } else {                                                    \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT ! -f -p tcp -m multiport --sports %u,%u -j DROP", (y), (z)); \
    }                                                           \
  }                                                             \
  system((cmd));                                                \
  memset((cmd), 0x0, sizeof((cmd)));                            \
  if ((v) == 6) {                                               \
    if ((x)) {                                                  \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u -p udp -m multiport --sports %u,%u -j DROP", (x), (y), (z)); \
    } else {                                                    \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -p udp -m multiport --sports %u,%u -j DROP", (y), (z)); \
    }                                                           \
  } else {                                                      \
    if ((x)) {                                                  \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u ! -f -p udp -m multiport --sports %u,%u -j DROP", (x), (y), (z)); \
    } else {                                                    \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT ! -f -p udp -m multiport --sports %u,%u -j DROP", (y), (z)); \
    }                                                           \
  }                                                             \
  system((cmd));                                                \
} else {                                                        \
  memset((cmd), 0x0, sizeof((cmd)));                            \
  if ((v) == 6) {                                               \
    if ((x)) {                                                  \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u -p tcp -m tcp --dport %u -j DROP", (x), (y)); \
    } else {                                                    \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -p tcp -m tcp --dport %u -j DROP", (y)); \
    }                                                           \
  } else {                                                      \
    if ((x)) {                                                  \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u ! -f -p tcp -m tcp --dport %u -j DROP", (x), (y)); \
    } else {                                                    \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT ! -f -p tcp -m tcp --dport %u -j DROP", (y)); \
    }                                                           \
  }                                                             \
  system((cmd));                                                \
  memset((cmd), 0x0, sizeof((cmd)));                            \
  if ((v) == 6) {                                               \
    if ((x)) {                                                  \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u -p udp -m udp --dport %u -j DROP", (x), (y)); \
    } else {                                                    \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -p udp -m udp --dport %u -j DROP", (y)); \
    }                                                           \
  } else {                                                      \
    if ((x)) {                                                  \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u ! -f -p udp -m udp --dport %u -j DROP", (x), (y)); \
    } else {                                                    \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT ! -f -p udp -m udp --dport %u -j DROP", (y)); \
    }                                                           \
  }                                                             \
  system((cmd));                                                \
}                                                               \
} while (0)

#define ACCESS_MGMT_IPTABLES_DP_ACCEPT(cmd, v, w, x, y, z) do { \
if ((z)) {                                                      \
  memset((cmd), 0x0, sizeof((cmd)));                            \
  if ((v) == 6) {                                               \
    if ((w)) {                                                  \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u %s -p tcp -m multiport --dports %u,%u -j ACCEPT", (w), (x), (y), (z)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u -p tcp -m multiport --dports %u,%u -j ACCEPT", (w), (y), (z)); \
    } else {                                                    \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT %s -p tcp -m multiport --dports %u,%u -j ACCEPT", (x), (y), (z)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -p tcp -m multiport --dports %u,%u -j ACCEPT", (y), (z)); \
    }                                                           \
  } else {                                                      \
    if ((w)) {                                                  \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u %s ! -f -p tcp -m multiport --dports %u,%u -j ACCEPT", (w), (x), (y), (z)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u ! -f -p tcp -m multiport --dports %u,%u -j ACCEPT", (w), (y), (z)); \
    } else {                                                    \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT %s ! -f -p tcp -m multiport --dports %u,%u -j ACCEPT", (x), (y), (z)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT ! -f -p tcp -m multiport --dports %u,%u -j ACCEPT", (y), (z)); \
    }                                                           \
  }                                                             \
  system((cmd));                                                \
  memset((cmd), 0x0, sizeof((cmd)));                            \
  if ((v) == 6) {                                               \
    if ((w)) {                                                  \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u %s -p udp -m multiport --dports %u,%u -j ACCEPT", (w), (x), (y), (z)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u -p udp -m multiport --dports %u,%u -j ACCEPT", (w), (y), (z)); \
    } else {                                                    \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT %s -p udp -m multiport --dports %u,%u -j ACCEPT", (x), (y), (z)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -p udp -m multiport --dports %u,%u -j ACCEPT", (y), (z)); \
    }                                                           \
  } else {                                                      \
    if ((w)) {                                                  \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u %s ! -f -p udp -m multiport --dports %u,%u -j ACCEPT", (w), (x), (y), (z)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u ! -f -p udp -m multiport --dports %u,%u -j ACCEPT", (w), (y), (z)); \
    } else {                                                    \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT %s ! -f -p udp -m multiport --dports %u,%u -j ACCEPT", (x), (y), (z)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT ! -f -p udp -m multiport --dports %u,%u -j ACCEPT", (y), (z)); \
    }                                                           \
  }                                                             \
  system((cmd));                                                \
} else {                                                        \
  memset((cmd), 0x0, sizeof((cmd)));                            \
  if ((v) == 6) {                                               \
    if ((w)) {                                                  \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u %s -p tcp -m tcp --dport %u -j ACCEPT", (w), (x), (y)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u -p tcp -m tcp --dport %u -j ACCEPT", (w), (y)); \
    } else {                                                    \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT %s -p tcp -m tcp --dport %u -j ACCEPT", (x), (y)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -p tcp -m tcp --dport %u -j ACCEPT", (y)); \
    }                                                           \
  } else {                                                      \
    if ((w)) {                                                  \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u %s ! -f -p tcp -m tcp --dport %u -j ACCEPT", (w), (x), (y)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u ! -f -p tcp -m tcp --dport %u -j ACCEPT", (w), (y)); \
    } else {                                                    \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT %s ! -f -p tcp -m tcp --dport %u -j ACCEPT", (x), (y)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT ! -f -p tcp -m tcp --dport %u -j ACCEPT", (y)); \
    }                                                           \
  }                                                             \
  system((cmd));                                                \
  memset((cmd), 0x0, sizeof((cmd)));                            \
  if ((v) == 6) {                                               \
    if ((w)) {                                                  \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u %s -p udp -m udp --dport %u -j ACCEPT", (w), (x), (y)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u -p udp -m udp --dport %u -j ACCEPT", (w), (y)); \
    } else {                                                    \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT %s -p udp -m udp --dport %u -j ACCEPT", (x), (y)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -p udp -m udp --dport %u -j ACCEPT", (y)); \
    }                                                           \
  } else {                                                      \
    if ((w)) {                                                  \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u %s ! -f -p udp -m udp --dport %u -j ACCEPT", (w), (x), (y)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u ! -f -p udp -m udp --dport %u -j ACCEPT", (w), (y)); \
    } else {                                                    \
      if (strlen((x)) > 0)                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT %s ! -f -p udp -m udp --dport %u -j ACCEPT", (x), (y)); \
      else                                                      \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT ! -f -p udp -m udp --dport %u -j ACCEPT", (y)); \
    }                                                           \
  }                                                             \
  system((cmd));                                                \
}                                                               \
} while (0)

#define ACCESS_MGMT_IPTABLES_DP_DROP(cmd, v, x, y, z)  do {     \
if ((z)) {                                                      \
  memset((cmd), 0x0, sizeof((cmd)));                            \
  if ((v) == 6) {                                               \
    if ((x)) {                                                  \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u -p tcp -m multiport --dports %u,%u -j DROP", (x), (y), (z)); \
    } else {                                                    \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -p tcp -m multiport --dports %u,%u -j DROP", (y), (z)); \
    }                                                           \
  } else {                                                      \
    if ((x)) {                                                  \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u ! -f -p tcp -m multiport --dports %u,%u -j DROP", (x), (y), (z)); \
    } else {                                                    \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT ! -f -p tcp -m multiport --dports %u,%u -j DROP", (y), (z)); \
    }                                                           \
  }                                                             \
  system((cmd));                                                \
  memset((cmd), 0x0, sizeof((cmd)));                            \
  if ((v) == 6) {                                               \
    if ((x)) {                                                  \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u -p udp -m multiport --dports %u,%u -j DROP", (x), (y), (z)); \
    } else {                                                    \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -p udp -m multiport --dports %u,%u -j DROP", (y), (z)); \
    }                                                           \
  } else {                                                      \
    if ((x)) {                                                  \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u ! -f -p udp -m multiport --dports %u,%u -j DROP", (x), (y), (z)); \
    } else {                                                    \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT ! -f -p udp -m multiport --dports %u,%u -j DROP", (y), (z)); \
    }                                                           \
  }                                                             \
  system((cmd));                                                \
} else {                                                        \
  memset((cmd), 0x0, sizeof((cmd)));                            \
  if ((v) == 6) {                                               \
    if ((x)) {                                                  \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u -p tcp -m tcp --dport %u -j DROP", (x), (y)); \
    } else {                                                    \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -p tcp -m tcp --dport %u -j DROP", (y)); \
    }                                                           \
  } else {                                                      \
    if ((x)) {                                                  \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u ! -f -p tcp -m tcp --dport %u -j DROP", (x), (y)); \
    } else {                                                    \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT ! -f -p tcp -m tcp --dport %u -j DROP", (y)); \
    }                                                           \
  }                                                             \
  system((cmd));                                                \
  memset((cmd), 0x0, sizeof((cmd)));                            \
  if ((v) == 6) {                                               \
    if ((x)) {                                                  \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -i vtss.vlan.%u -p udp -m udp --dport %u -j DROP", (x), (y)); \
    } else {                                                    \
        snprintf((cmd), sizeof((cmd)), "ip6tables -A INPUT -p udp -m udp --dport %u -j DROP", (y)); \
    }                                                           \
  } else {                                                      \
    if ((x)) {                                                  \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT -i vtss.vlan.%u ! -f -p udp -m udp --dport %u -j DROP", (x), (y)); \
    } else {                                                    \
        snprintf((cmd), sizeof((cmd)), "iptables -A INPUT ! -f -p udp -m udp --dport %u -j DROP", (y)); \
    }                                                           \
  }                                                             \
  system((cmd));                                                \
}                                                               \
} while (0)

#define ACCESS_MGMT_IPTABLES_FLUSH(v)               do {        \
  system("iptables --flush INPUT");                             \
  if ((v) != 0) system("ip6tables --flush INPUT");              \
} while (0)

static BOOL ACCESS_MGMT_iptables_do_drop(
    access_mgmt_conf_t  *conf,
    u32                 chk,
    access_mgmt_entry_t *cmp
)
{
    u32                 idx;
    access_mgmt_entry_t *ety;
    BOOL                do_drop;

    if (!conf || !cmp) {
        return FALSE;
    }

    do_drop = TRUE;
    for (idx = chk + 1; !(idx > ACCESS_MGMT_MAX_ENTRIES); idx++) {
        ety = &conf->entry[idx];
        if (!ety->valid || !ety->vid ||
            ety->vid != cmp->vid ||
            ety->entry_type != cmp->entry_type) {
            continue;
        }

        do_drop = FALSE;
        break;
    }

    return do_drop;
}

static void ACCESS_MGMT_iptables_apply(access_mgmt_conf_t *conf)
{
    if (conf && conf->mode) {
        char                cmd[256], adrs[128], abufs[40], abufe[40];
        u8                  ver, idx;
        BOOL                do_drop;
        access_mgmt_entry_t *ety;

        for (idx = ACCESS_MGMT_ACCESS_ID_START; !(idx > ACCESS_MGMT_MAX_ENTRIES); idx++) {
            ety = &conf->entry[idx];
            if (!ety->valid || !ety->vid) {
                continue;
            }

            ver = 0;
            memset(adrs, 0x0, sizeof(adrs));
            if (ety->entry_type == ACCESS_MGMT_ENTRY_TYPE_IPV6) {
#ifdef VTSS_SW_OPTION_IPV6
                ver = 6;
                snprintf(adrs, sizeof(adrs), "-m iprange --src-range %s-%s", misc_ipv6_txt(&ety->start_ipv6, abufs), misc_ipv6_txt(&ety->end_ipv6, abufe));
#else
                continue;
#endif /* VTSS_SW_OPTION_IPV6 */
            } else {
                ver = 4;
                snprintf(adrs, sizeof(adrs), "-m iprange --src-range %s-%s", misc_ipv4_txt(ety->start_ip, abufs), misc_ipv4_txt(ety->end_ip, abufe));
            }

            do_drop = ACCESS_MGMT_iptables_do_drop(conf, idx, ety);
            T_I("DO(%sDROP) ip%stables with service_type %d", do_drop ? "&" : "!", ver == 6 ? "6" : "", ety->service_type);

            if (ety->service_type & ACCESS_MGMT_SERVICES_TYPE_SNMP) {
                ACCESS_MGMT_IPTABLES_DP_ACCEPT(cmd, ver, ety->vid, adrs, UDP_PROTO_SNMP, 0);
                T_D("SNMP");
            }
            if (do_drop) {
                ACCESS_MGMT_IPTABLES_DP_DROP(cmd, ver, ety->vid, UDP_PROTO_SNMP, 0);
                T_D("DROP SNMP on IPIF-%u", ety->vid);
            }

            if (ety->service_type & ACCESS_MGMT_SERVICES_TYPE_TELNET) {
                ACCESS_MGMT_IPTABLES_DP_ACCEPT(cmd, ver, ety->vid, adrs, TCP_PROTO_SSH, TCP_PROTO_TELNET);
                T_D("SSH/TELNET");
            }
            if (do_drop) {
                ACCESS_MGMT_IPTABLES_DP_DROP(cmd, ver, ety->vid, TCP_PROTO_SSH, TCP_PROTO_TELNET);
                T_D("DROP SSH/TELNET on IPIF-%u", ety->vid);
            }

            if (ety->service_type & ACCESS_MGMT_SERVICES_TYPE_WEB) {
                ACCESS_MGMT_IPTABLES_DP_ACCEPT(cmd, ver, ety->vid, adrs, TCP_PROTO_HTTP, TCP_PROTO_HTTPS);
                T_D("HTTP/HTTPS");
            }
            if (do_drop) {
                ACCESS_MGMT_IPTABLES_DP_DROP(cmd, ver, ety->vid, TCP_PROTO_HTTP, TCP_PROTO_HTTPS);
                T_D("DROP HTTP/HTTPS on IPIF-%u", ety->vid);
            }
        }

        ACCESS_MGMT_IPTABLES_DP_DROP(cmd, 4, 0, UDP_PROTO_SNMP, 0);
        ACCESS_MGMT_IPTABLES_DP_DROP(cmd, 6, 0, UDP_PROTO_SNMP, 0);
        ACCESS_MGMT_IPTABLES_DP_DROP(cmd, 4, 0, TCP_PROTO_SSH, TCP_PROTO_TELNET);
        ACCESS_MGMT_IPTABLES_DP_DROP(cmd, 6, 0, TCP_PROTO_SSH, TCP_PROTO_TELNET);
        ACCESS_MGMT_IPTABLES_DP_DROP(cmd, 4, 0, TCP_PROTO_HTTP, TCP_PROTO_HTTPS);
        ACCESS_MGMT_IPTABLES_DP_DROP(cmd, 6, 0, TCP_PROTO_HTTP, TCP_PROTO_HTTPS);
    }
}

static mesa_rc ACCESS_MGMT_internal_iptables_setup(void)
{
    char                                cmd[296], adrs[200], sabufs[40], sabufe[40], dabufs[40], dabufe[40];
    u8                                  idx, ver;
    access_mgmt_inter_module_entry_t    *internal_setting, *ety;

    if (VTSS_MALLOC_CAST(internal_setting, sizeof(access_mgmt_inter_module_entry_t) * ACCESS_MGMT_MAX_ENTRIES) == NULL) {
        return VTSS_RC_ERROR;
    }

    ACCESS_MGMT_CRIT_ENTER();
    memcpy(internal_setting, ACCESS_MGMT_global.access_mgmt_internal, sizeof(ACCESS_MGMT_global.access_mgmt_internal));
    ACCESS_MGMT_CRIT_EXIT();

    for (idx = 0; idx < ACCESS_MGMT_MAX_ENTRIES; ++idx) {
        ety = &internal_setting[idx];
        if (ety->source == ACCESS_MGMT_INTERNAL_TYPE_NONE) {
            continue;
        }

        ver = 0;
        memset(adrs, 0x0, sizeof(adrs));
        if (ety->start_src.type == MESA_IP_TYPE_IPV6 || ety->start_dst.type == MESA_IP_TYPE_IPV6) {
#ifdef VTSS_SW_OPTION_IPV6
            ver = 6;
            if (ety->start_src.type == MESA_IP_TYPE_IPV6 && ety->start_dst.type == MESA_IP_TYPE_IPV6) {
                snprintf(adrs, sizeof(adrs), "-m iprange --src-range %s-%s --dst-range %s-%s", misc_ipv6_txt(&ety->start_src.addr.ipv6, sabufs), misc_ipv6_txt(&ety->end_src.addr.ipv6, sabufe), misc_ipv6_txt(&ety->start_dst.addr.ipv6, dabufs), misc_ipv6_txt(&ety->end_dst.addr.ipv6, dabufe));
            } else {
                if (ety->start_src.type == MESA_IP_TYPE_IPV6) {
                    snprintf(adrs, sizeof(adrs), "-m iprange --src-range %s-%s", misc_ipv6_txt(&ety->start_src.addr.ipv6, sabufs), misc_ipv6_txt(&ety->end_src.addr.ipv6, sabufe));
                }
                if (ety->start_dst.type == MESA_IP_TYPE_IPV6) {
                    snprintf(adrs, sizeof(adrs), "-m iprange --dst-range %s-%s", misc_ipv6_txt(&ety->start_dst.addr.ipv6, dabufs), misc_ipv6_txt(&ety->end_dst.addr.ipv6, dabufe));
                }
            }
#else
            continue;
#endif /* VTSS_SW_OPTION_IPV6 */
        } else if (ety->start_src.type == MESA_IP_TYPE_IPV4 || ety->start_dst.type == MESA_IP_TYPE_IPV4) {
            ver = 4;
            if (ety->start_src.type == MESA_IP_TYPE_IPV4 && ety->start_dst.type == MESA_IP_TYPE_IPV4) {
                snprintf(adrs, sizeof(adrs), "-m iprange --src-range %s-%s --dst-range %s-%s", misc_ipv4_txt(ety->start_src.addr.ipv4, sabufs), misc_ipv4_txt(ety->end_src.addr.ipv4, sabufe), misc_ipv4_txt(ety->start_dst.addr.ipv4, dabufs), misc_ipv4_txt(ety->end_dst.addr.ipv4, dabufe));
            } else {
                if (ety->start_src.type == MESA_IP_TYPE_IPV4) {
                    snprintf(adrs, sizeof(adrs), "-m iprange --src-range %s-%s", misc_ipv4_txt(ety->start_src.addr.ipv4, sabufs), misc_ipv4_txt(ety->end_src.addr.ipv4, sabufe));
                }
                if (ety->start_dst.type == MESA_IP_TYPE_IPV4) {
                    snprintf(adrs, sizeof(adrs), "-m iprange --dst-range %s-%s", misc_ipv4_txt(ety->start_dst.addr.ipv4, dabufs), misc_ipv4_txt(ety->end_dst.addr.ipv4, dabufe));
                }
            }
        }

        if (ver > 0) {
            if (ety->operation) {
                if (ety->start_dport) {
                    ACCESS_MGMT_IPTABLES_DP_ACCEPT(cmd, ver, ety->vidx, adrs, ety->start_dport, ety->end_dport);
                }
                if (ety->start_sport) {
                    ACCESS_MGMT_IPTABLES_SP_ACCEPT(cmd, ver, ety->vidx, adrs, ety->start_sport, ety->end_sport);
                }
            } else {
                if (ety->start_dport) {
                    ACCESS_MGMT_IPTABLES_DP_DROP(cmd, ver, ety->vidx, ety->start_dport, ety->end_dport);
                }
                if (ety->start_sport) {
                    ACCESS_MGMT_IPTABLES_SP_DROP(cmd, ver, ety->vidx, ety->start_sport, ety->end_sport);
                }
            }
        } else {
            if (ety->operation) {
                if (ety->start_dport) {
                    ACCESS_MGMT_IPTABLES_DP_ACCEPT(cmd, 4, ety->vidx, adrs, ety->start_dport, ety->end_dport);
                    ACCESS_MGMT_IPTABLES_DP_ACCEPT(cmd, 6, ety->vidx, adrs, ety->start_dport, ety->end_dport);
                }
                if (ety->start_sport) {
                    ACCESS_MGMT_IPTABLES_SP_ACCEPT(cmd, 4, ety->vidx, adrs, ety->start_sport, ety->end_sport);
                    ACCESS_MGMT_IPTABLES_SP_ACCEPT(cmd, 6, ety->vidx, adrs, ety->start_sport, ety->end_sport);
                }
            } else {
                if (ety->start_dport) {
                    ACCESS_MGMT_IPTABLES_DP_DROP(cmd, 4, ety->vidx, ety->start_dport, ety->end_dport);
                    ACCESS_MGMT_IPTABLES_DP_DROP(cmd, 6, ety->vidx, ety->start_dport, ety->end_dport);
                }
                if (ety->start_sport) {
                    ACCESS_MGMT_IPTABLES_SP_DROP(cmd, 4, ety->vidx, ety->start_sport, ety->end_sport);
                    ACCESS_MGMT_IPTABLES_SP_DROP(cmd, 6, ety->vidx, ety->start_sport, ety->end_sport);
                }
            }
        }
    }

    VTSS_FREE(internal_setting);

    return VTSS_RC_OK;
}

static mesa_rc ACCESS_MGMT_ip_filter_config_iptables(void)
{
    BOOL                do_internal, current_admin;
    access_mgmt_conf_t  *new_conf = NULL;

    if (VTSS_MALLOC_CAST(new_conf, sizeof(access_mgmt_conf_t)) == NULL) {
        return VTSS_RC_ERROR;
    }

    ACCESS_MGMT_CRIT_ENTER();
    memcpy(new_conf, &ACCESS_MGMT_global.access_mgmt_conf, sizeof(access_mgmt_conf_t));
    do_internal = (ACCESS_MGMT_global.access_mgmt_internal_count > 0);
    current_admin = ACCESS_MGMT_state;
    ACCESS_MGMT_CRIT_EXIT();

    if (do_internal || current_admin || new_conf->mode != 0) {
#ifdef VTSS_SW_OPTION_IPV6
        ACCESS_MGMT_IPTABLES_FLUSH(1);
#else
        ACCESS_MGMT_IPTABLES_FLUSH(0);
#endif /* VTSS_SW_OPTION_IPV6 */
    }

    if ((current_admin = (new_conf->mode != 0)) == TRUE) {
        ACCESS_MGMT_iptables_apply(new_conf);
    }

    ACCESS_MGMT_CRIT_ENTER();
    ACCESS_MGMT_state = current_admin;
    ACCESS_MGMT_CRIT_EXIT();

    if (do_internal && ACCESS_MGMT_internal_iptables_setup() != VTSS_RC_OK) {
        T_D("Failed to setup iptables for internal module!");
    }

    VTSS_FREE(new_conf);

    return VTSS_RC_OK;
}

#ifdef VTSS_SW_OPTION_PACKET
static void *rcv_filter_id = NULL;

static BOOL ACCESS_MGMT_rx_callback(
    void                        *contxt,
    const u8                    *const frm,
    const mesa_packet_rx_info_t *const rx_info
)
{
    ip_stack_glue_frame_info_t  frm_info;
    u32                         frm_offset, frm_len, ipv;
    u16                         *ether_type;
    struct tcphdr               *tcp_hdr;
    struct udphdr               *udp_hdr;

    if (!frm || !rx_info) {
        return FALSE;
    }

    frm_offset = 12;    // Skip DMAC + SMAC
    ether_type = (u16 *)(frm + frm_offset);
    T_N("ETH_TYPE: 0x%X", ntohs(*ether_type));
    if (ntohs(*ether_type) != 0x0800) {
#ifdef VTSS_SW_OPTION_IPV6
        if (ntohs(*ether_type) != 0x86DD) {
            return FALSE;
        }
#else
        return FALSE;
#endif /* VTSS_SW_OPTION_IPV6 */
    }

    frm_offset += 2;
    ipv = ntohl(*((u32 *)(frm + frm_offset))) >> 28;
    frm_len = rx_info->length - frm_offset;
    memset(&frm_info, 0x0, sizeof(ip_stack_glue_frame_info_t));
    tcp_hdr = NULL;
    udp_hdr = NULL;
    T_N("VER:%u / LEN:%u", ipv, frm_len);
    if (ipv == 4) {
        if (frm_len < (frm_offset + sizeof(struct ip))) {
            return FALSE;
        }

        frm_offset += 6;
        // Ignore fragments
        if (ntohs(*((u16 *)(frm + frm_offset))) & 0x1FFF) {
            return FALSE;
        }

        frm_offset += 3;
        frm_info.ip_protocol = *(frm + frm_offset);
        frm_offset += 3;
        frm_info.sip = ntohl(*((u32 *)(frm + frm_offset)));
        frm_offset += 4;
        frm_info.dip = ntohl(*((u32 *)(frm + frm_offset)));
        frm_offset += 4;
        if (frm_info.ip_protocol == 0x6) {
            if (frm_len < sizeof(struct ip) + sizeof(struct tcphdr)) {
                return FALSE;
            }
            tcp_hdr = (struct tcphdr *)(frm + frm_offset);
        } else if (frm_info.ip_protocol == 0x11) {
            if (frm_len < sizeof(struct ip) + sizeof(struct udphdr)) {
                return FALSE;
            }
            udp_hdr = (struct udphdr *)(frm + frm_offset);
        } else {
            return FALSE;
        }

        frm_info.eth_type = ETYPE_IPV4;
        frm_info.ip_version = 4;
#ifdef VTSS_SW_OPTION_IPV6
    } else if (ipv == 6) {
        if (frm_len < (frm_offset + sizeof(struct ip6_hdr))) {
            return FALSE;
        }

        frm_offset += 6;
        frm_info.ip_protocol = *(frm + frm_offset);
        frm_offset += 2;
        memcpy(frm_info.sip_v6, frm + frm_offset, sizeof(mesa_ipv6_t));
        frm_offset += 16;
        memcpy(frm_info.dip_v6, frm + frm_offset, sizeof(mesa_ipv6_t));
        frm_offset += 16;
        if (frm_info.ip_protocol == 0x6) {
            if (frm_len < sizeof(struct ip6_hdr) + sizeof(struct tcphdr)) {
                return FALSE;
            }
            tcp_hdr = (struct tcphdr *)(frm + frm_offset);
        } else if (frm_info.ip_protocol == 0x11) {
            if (frm_len < sizeof(struct ip6_hdr) + sizeof(struct udphdr)) {
                return FALSE;
            }
            udp_hdr = (struct udphdr *)(frm + frm_offset);
        } else {
            return FALSE;
        }

        frm_info.eth_type = ETYPE_IPV6;
        frm_info.ip_version = 6;
#endif /* VTSS_SW_OPTION_IPV6 */
    } else {
        return FALSE;
    }

    T_N("Parse %s port", tcp_hdr ? "TCP" : (udp_hdr ? "UDP" : "OTHER"));
    if (tcp_hdr) {
        frm_info.sport = ntohs(tcp_hdr->source);
        frm_info.dport = ntohs(tcp_hdr->dest);
    }
    if (udp_hdr) {
#if defined(__FAVOR_BSD) || defined(VTSS_SW_OPTION_DHCP_RELAY)
        frm_info.sport = ntohs(udp_hdr->uh_sport);
        frm_info.dport = ntohs(udp_hdr->uh_dport);
#else
        frm_info.sport = ntohs(udp_hdr->source);
        frm_info.dport = ntohs(udp_hdr->dest);
#endif
    }

    /* These protocols/applications use the same reserved port for both TCP & UDP */
    if (frm_info.dport == TCP_PROTO_HTTP) {
        frm_info.appl_type = IP_STACK_GLUE_APPL_TYPE_HTTP;
    } else if (frm_info.dport == TCP_PROTO_HTTPS) {
        frm_info.appl_type = IP_STACK_GLUE_APPL_TYPE_HTTPS;
    } else if (frm_info.dport == UDP_PROTO_SNMP) {
        frm_info.appl_type = IP_STACK_GLUE_APPL_TYPE_SNMP;
    } else if (frm_info.dport == TCP_PROTO_SSH) {
        frm_info.appl_type = IP_STACK_GLUE_APPL_TYPE_SSH;
    } else if (frm_info.dport == TCP_PROTO_TELNET) {
        frm_info.appl_type = IP_STACK_GLUE_APPL_TYPE_TELNET;
    } else {
        return FALSE;
    }

    T_D("Check VLAN-%u %s (SRCP:%u / DSTP:%u)",
        rx_info->tag.vid,
        access_mgmt_service_type_txt(frm_info.appl_type),
        frm_info.sport,
        frm_info.dport);

    return ACCESS_MGMT_ip_stack_glue_callback(rx_info->tag.vid, &frm_info);
}
#endif /* VTSS_SW_OPTION_PACKET */

static mesa_rc ACCESS_MGMT_ip_filter_config_callback(BOOL reg_filter)
{
    mesa_rc rc = VTSS_RC_OK;

#ifdef VTSS_SW_OPTION_PACKET
    if (reg_filter) {
        packet_rx_filter_t filter;

        packet_rx_filter_init(&filter);
        filter.modid    = VTSS_MODULE_ID_ACCESS_MGMT;
        filter.match    = PACKET_RX_FILTER_MATCH_IP_ANY | PACKET_RX_FILTER_MATCH_VLAN_TAG_ANY;
        filter.prio     = PACKET_RX_FILTER_PRIO_LOW;
        filter.cb       = ACCESS_MGMT_rx_callback;

        if (rcv_filter_id) {
            rc = packet_rx_filter_change(&filter, &rcv_filter_id);
        } else {
            rc = packet_rx_filter_register(&filter, &rcv_filter_id);
        }
    } else {
        if (rcv_filter_id != NULL &&
            (rc = packet_rx_filter_unregister(rcv_filter_id)) == VTSS_RC_OK) {
            rcv_filter_id = NULL;
        }
    }
#endif /* VTSS_SW_OPTION_PACKET */

    if (rc == VTSS_RC_OK) {
        vtss_flag_setbits(&ACCESS_MGMT_thread_flag, ACCESS_MGMT_EVENT_POKE);
    }

    return rc;
}

void ACCESS_MGMT_thread(vtss_addrword_t data)
{
    vtss_flag_value_t   events;

    /* Initialize EVENT groups */
    vtss_flag_init(&ACCESS_MGMT_thread_flag);

    while (TRUE) {
        events = vtss_flag_wait(&ACCESS_MGMT_thread_flag, ACCESS_MGMT_EVENT_ANY, VTSS_FLAG_WAITMODE_OR_CLR);

        T_D("ACCESS_MGMT_EVENT: %d", events);

        if (events & ACCESS_MGMT_EVENT_EXIT) {
            break;
        }

        if (events & ACCESS_MGMT_EVENT_SLOW) {
            VTSS_OS_MSLEEP(1973);
            vtss_flag_setbits(&ACCESS_MGMT_thread_flag, ACCESS_MGMT_EVENT_POKE);
        }

        if (events & ACCESS_MGMT_EVENT_POKE) {
            if (ACCESS_MGMT_ip_filter_config_iptables() != VTSS_RC_OK) {
                vtss_flag_setbits(&ACCESS_MGMT_thread_flag, ACCESS_MGMT_EVENT_SLOW);
            }
        }
    }

    vtss_flag_destroy(&ACCESS_MGMT_thread_flag);
    T_W("ACCESS_MGMT_EVENT_EXIT");
}

static int ACCESS_MGMT_register_ip_stack_glue_flag;
static mesa_rc ACCESS_MGMT_ip_filter_set(void)
{
    int     reg_filter = 0;
    mesa_rc rc = VTSS_RC_OK;

    ACCESS_MGMT_CRIT_ENTER();
    if (ACCESS_MGMT_register_ip_stack_glue_flag < 1 && ACCESS_MGMT_global.access_mgmt_conf.mode) {
        reg_filter = 1;
    } else if (ACCESS_MGMT_register_ip_stack_glue_flag > 0 && !ACCESS_MGMT_global.access_mgmt_conf.mode) {
        reg_filter = -1;
    }
    ACCESS_MGMT_CRIT_EXIT();

    if (reg_filter != 0) {
        if (reg_filter < 0) {
            /* Unregister for IP filtering callbacks */
            if ((rc = ACCESS_MGMT_ip_filter_config_callback(FALSE)) != VTSS_RC_OK) {
                T_E("Failed to unregisterer filter function (we are not registered in the first place");
            }
        } else {
            /* Register for IP filtering callbacks */
            if ((rc = ACCESS_MGMT_ip_filter_config_callback(TRUE)) != VTSS_RC_OK) {
                T_E("Failed to register filter function");
            }
        }

        if (rc == VTSS_RC_OK) {
            ACCESS_MGMT_CRIT_ENTER();
            ACCESS_MGMT_register_ip_stack_glue_flag = reg_filter;
            ACCESS_MGMT_CRIT_EXIT();
        }
    }

    if (reg_filter == 0) {
        vtss_flag_setbits(&ACCESS_MGMT_thread_flag, ACCESS_MGMT_EVENT_POKE);
    }

    return rc;
}

static void ACCESS_MGMT_initialize(void)
{
    T_D("enter");

    /* Initialize access_mgmt configuration */
    ACCESS_MGMT_CRIT_ENTER();
    ACCESS_MGMT_register_ip_stack_glue_flag = 0;
    ACCESS_MGMT_default_set(&ACCESS_MGMT_global.access_mgmt_conf);
    ACCESS_MGMT_CRIT_EXIT();
    ACCESS_MGMT_internal_db_reset();

#ifdef VTSS_SW_OPTION_IPV6
    ACCESS_MGMT_IPTABLES_FLUSH(1);
#else
    ACCESS_MGMT_IPTABLES_FLUSH(0);
#endif /* VTSS_SW_OPTION_IPV6 */

    ACCESS_MGMT_state = FALSE;
    vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                       ACCESS_MGMT_thread,
                       0,
                       "ACCESS_MGMT",
                       nullptr,
                       0,
                       &ACCESS_MGMT_thread_handle,
                       &ACCESS_MGMT_thread_block);

    T_D("exit");
}

static mesa_rc ACCESS_MGMT_conf_apply(vtss_isid_t isid)
{
    mesa_rc rc = VTSS_RC_OK;

    if (msg_switch_is_primary()) {
        if (isid != VTSS_ISID_GLOBAL &&
            isid != VTSS_ISID_LOCAL &&
            msg_switch_is_local(isid)) {
            return rc;
        }

        if ((rc = ACCESS_MGMT_ip_filter_set()) != VTSS_RC_OK) {
            T_D("Failed to call IP filter!");
        } else {
            /* Need to propagate configuration to secondary switch. */
        }
    }

    return rc;
}

/* Reset access_mgmt configuration */
static void ACCESS_MGMT_conf_reset(void)
{
    access_mgmt_conf_t  new_access_mgmt_conf;

    T_D("enter");

    /* Use default values */
    ACCESS_MGMT_default_set(&new_access_mgmt_conf);

    ACCESS_MGMT_CRIT_ENTER();
    if (ACCESS_MGMT_compare_cfg(&ACCESS_MGMT_global.access_mgmt_conf, &new_access_mgmt_conf) != 0) {
        ACCESS_MGMT_global.access_mgmt_conf = new_access_mgmt_conf;
    }
    ACCESS_MGMT_CRIT_EXIT();
    ACCESS_MGMT_internal_db_reset();

    if (ACCESS_MGMT_conf_apply(VTSS_ISID_GLOBAL) != VTSS_RC_OK) {
        T_D("Failed to apply configuration!");
    }

    T_D("exit");
}

static BOOL ACCESS_MGMT_conf_sanity(access_mgmt_conf_t *conf)
{
    u32                 idx;
    access_mgmt_entry_t *ety;
    mesa_ip_addr_t      ipas, ipae;

    if (!conf) {
        return FALSE;
    }

    for (idx = ACCESS_MGMT_ACCESS_ID_START; !(idx > ACCESS_MGMT_MAX_ENTRIES); idx++) {
        ety = &conf->entry[idx];
        if (!ety->valid || !ety->vid) {
            continue;
        }

        if (ety->vid >= VTSS_APPL_VLAN_ID_MIN &&
            ety->vid <= VTSS_APPL_VLAN_ID_MAX) {
            memset(&ipas, 0x0, sizeof(mesa_ip_addr_t));
            memset(&ipae, 0x0, sizeof(mesa_ip_addr_t));

            if (ety->entry_type == ACCESS_MGMT_ENTRY_TYPE_IPV4) {
                if (ety->start_ip > ety->end_ip) {
                    return FALSE;
                }
                ipas.type = ipae.type = MESA_IP_TYPE_IPV4;
                ipas.addr.ipv4 = ety->start_ip;
                ipae.addr.ipv4 = ety->end_ip;
#ifdef VTSS_SW_OPTION_IPV6
            } else if (ety->entry_type == ACCESS_MGMT_ENTRY_TYPE_IPV6) {
                if (memcmp(&ety->start_ipv6, &ety->end_ipv6, sizeof(mesa_ipv6_t)) > 0) {
                    return FALSE;
                }
                ipas.type = ipae.type = MESA_IP_TYPE_IPV6;
                ipas.addr.ipv6 = ety->start_ipv6;
                ipae.addr.ipv6 = ety->end_ipv6;
#endif /* VTSS_SW_OPTION_IPV6 */
            } else {
                ipas.type = ipae.type = MESA_IP_TYPE_NONE;
            }

            if (!ACCESS_MGMT_ipa_valid(&ipas) || !ACCESS_MGMT_ipa_valid(&ipae)) {
                return FALSE;
            }
        } else {
            return FALSE;
        }
    }

    return TRUE;
}

/****************************************************************************/
/*  Management functions                                                    */
/****************************************************************************/

/* access_mgmt error text */
const char *access_mgmt_error_txt(mesa_rc rc)
{
    const char *txt;

    switch (rc) {
    case ACCESS_MGMT_ERROR_GEN:
        txt = "access_mgmt generic error";
        break;
    case ACCESS_MGMT_ERROR_PARM:
        txt = "access_mgmt parameter error";
        break;
    case ACCESS_MGMT_ERROR_STACK_STATE:
        txt = "access_mgmt stack state error";
        break;
    case ACCESS_MGMT_ERROR_NULL_PARM:
        txt = "Parameter is invalid (is NULL).";
        break;
    case ACCESS_MGMT_ERROR_DUPLICATED_CONTENT:
        txt = "Content duplicated.";
        break;
    case ACCESS_MGMT_ERROR_INVALID_SERVICE:
        txt = "Invalid service type.";
        break;
    case ACCESS_MGMT_ERROR_NOT_IPV4_INPUT_TYPE:
        txt = "Type is not IPv4 as expected.";
        break;
    default:
        txt = "access_mgmt unknown error";
        break;
    }
    return txt;
}

/* Get access_mgmt configuration */
mesa_rc access_mgmt_conf_get(access_mgmt_conf_t *conf)
{
    T_D("enter");

    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return ACCESS_MGMT_ERROR_STACK_STATE;
    }
    if (!conf) {
        T_D("exit");
        return ACCESS_MGMT_ERROR_NULL_PARM;
    }

    ACCESS_MGMT_CRIT_ENTER();
    *conf = ACCESS_MGMT_global.access_mgmt_conf;
    ACCESS_MGMT_CRIT_EXIT();

    T_D("exit");

    return VTSS_RC_OK;
}

/* Set access_mgmt configuration */
mesa_rc access_mgmt_conf_set(access_mgmt_conf_t *conf)
{
    mesa_rc rc;
    int     changed;

    T_D("enter");

    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return ACCESS_MGMT_ERROR_STACK_STATE;
    }
    if (!ACCESS_MGMT_conf_sanity(conf)) {
        T_D("exit");
        return VTSS_RC_ERROR;
    }

    changed = 0;
    rc = VTSS_RC_OK;
    ACCESS_MGMT_CRIT_ENTER();
    if ((changed = ACCESS_MGMT_compare_cfg(&ACCESS_MGMT_global.access_mgmt_conf, conf)) != 0) {
        ACCESS_MGMT_global.access_mgmt_conf = *conf;
    }
    ACCESS_MGMT_CRIT_EXIT();
    if (changed != 0) {
        rc = ACCESS_MGMT_conf_apply(VTSS_ISID_GLOBAL);
    }

    T_D("exit");

    return rc;
}

/* Module start */
static mesa_rc ACCESS_MGMT_start(void)
{
    mesa_rc rc;

    T_D("enter");

    rc = ACCESS_MGMT_conf_apply(VTSS_ISID_LOCAL);

    T_D("exit");

    return rc;
}

static BOOL ACCESS_MGMT_internal_entry_sanity(
    const access_mgmt_inter_module_entry_t  *const entry,
    BOOL                                    set
)
{
    if (!entry) {
        return FALSE;
    }

    if (entry->source == ACCESS_MGMT_INTERNAL_TYPE_NONE) {
        T_D("Invalid source: %d", entry->source);
        return FALSE;
    }
    if (!set) {
        return TRUE;
    }

    if (entry->protocol > ACCESS_MGMT_PROTOCOL_OTHER) {
        T_D("Invalid protocol: %d", entry->protocol);
        return FALSE;
    }

    if (entry->start_src.type != entry->start_dst.type &&
        entry->start_src.type != MESA_IP_TYPE_NONE &&
        entry->start_dst.type != MESA_IP_TYPE_NONE) {
        T_D("Invalid SRC<>DST address type: %d<>%d", entry->start_src.type, entry->start_dst.type);
        return FALSE;
    }

    if (entry->start_src.type != MESA_IP_TYPE_NONE) {
        if (entry->end_src.type != entry->start_src.type) {
            T_D("Invalid START<>END source address type: %d<>%d", entry->start_src.type, entry->end_src.type);
            return FALSE;
        }
        if (entry->start_src.type == MESA_IP_TYPE_IPV4) {
            if (entry->start_src.addr.ipv4 > entry->end_src.addr.ipv4) {
                T_D("SourceIPv4 START > END");
                return FALSE;
            }
        } else if (entry->start_src.type == MESA_IP_TYPE_IPV6) {
            if (memcmp(&entry->start_src.addr.ipv6, &entry->end_src.addr.ipv6, sizeof(mesa_ipv6_t)) > 0) {
                T_D("SourceIPv6 START > END");
                return FALSE;
            }
        } else {
            T_D("Unknown source address type: %d", entry->start_src.type);
            return FALSE;
        }
    }

    if (entry->start_dst.type != MESA_IP_TYPE_NONE) {
        if (entry->end_dst.type != entry->start_dst.type) {
            T_D("Invalid START<>END destination address type: %d<>%d", entry->start_dst.type, entry->end_dst.type);
            return FALSE;
        }
        if (entry->start_dst.type == MESA_IP_TYPE_IPV4) {
            if (entry->start_dst.addr.ipv4 > entry->end_dst.addr.ipv4) {
                T_D("DestinationIPv4 START > END");
                return FALSE;
            }
        } else if (entry->start_dst.type == MESA_IP_TYPE_IPV6) {
            if (memcmp(&entry->start_dst.addr.ipv6, &entry->end_dst.addr.ipv6, sizeof(mesa_ipv6_t)) > 0) {
                T_D("DestinationIPv6 START > END");
                return FALSE;
            }
        } else {
            T_D("Unknown destination address type: %d", entry->start_dst.type);
            return FALSE;
        }
    }

    if (entry->start_sport != 0) {
        if (entry->end_sport != 0 && entry->start_sport > entry->end_sport) {
            T_D("Invalid START<>END source protocol port: %u<>%u", entry->start_sport, entry->end_sport);
            return FALSE;
        }
    }

    if (entry->start_dport != 0) {
        if (entry->end_dport != 0 && entry->start_dport > entry->end_dport) {
            T_D("Invalid START<>END destination protocol port: %u<>%u", entry->start_dport, entry->end_dport);
            return FALSE;
        }
    }

    return TRUE;
}

/* Internal (Inter-Module) access management entry add */
mesa_rc access_mgmt_internal_entry_add(const access_mgmt_inter_module_entry_t *const entry)
{
    u32                                 idx;
    access_mgmt_inter_module_entry_t    ety;

    if (!entry) {
        return VTSS_RC_ERROR;
    }

    if (!ACCESS_MGMT_internal_entry_sanity(entry, TRUE)) {
        return VTSS_RC_ERROR;
    }

    ety.source = entry->source;
    if (access_mgmt_internal_entry_get(&ety) == VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    ACCESS_MGMT_CRIT_ENTER();
    for (idx = 0; idx < ACCESS_MGMT_MAX_ENTRIES; ++idx) {
        if (ACCESS_MGMT_global.access_mgmt_internal[idx].source == ACCESS_MGMT_INTERNAL_TYPE_NONE) {
            memcpy(&ACCESS_MGMT_global.access_mgmt_internal[idx], entry, sizeof(access_mgmt_inter_module_entry_t));
            ACCESS_MGMT_global.access_mgmt_internal_count++;
            vtss_flag_setbits(&ACCESS_MGMT_thread_flag, ACCESS_MGMT_EVENT_POKE);
            ACCESS_MGMT_CRIT_EXIT();
            return VTSS_RC_OK;
        }
    }
    ACCESS_MGMT_CRIT_EXIT();

    return VTSS_RC_ERROR;
}

/* Internal (Inter-Module) access management entry delete */
mesa_rc access_mgmt_internal_entry_del(const access_mgmt_inter_module_entry_t *const entry)
{
    u32                                 idx;
    access_mgmt_inter_module_entry_t    *ety;

    if (!entry) {
        return VTSS_RC_ERROR;
    }

    if (!ACCESS_MGMT_internal_entry_sanity(entry, FALSE)) {
        return VTSS_RC_ERROR;
    }

    ety = NULL;
    ACCESS_MGMT_CRIT_ENTER();
    for (idx = 0; idx < ACCESS_MGMT_MAX_ENTRIES; ++idx) {
        if (ACCESS_MGMT_global.access_mgmt_internal[idx].source == entry->source) {
            ety = &ACCESS_MGMT_global.access_mgmt_internal[idx];
            memset(ety, 0x0, sizeof(access_mgmt_inter_module_entry_t));
            ety->source = ACCESS_MGMT_INTERNAL_TYPE_NONE;
            ACCESS_MGMT_global.access_mgmt_internal_count--;
            vtss_flag_setbits(&ACCESS_MGMT_thread_flag, ACCESS_MGMT_EVENT_POKE);
            break;
        }
    }
    ACCESS_MGMT_CRIT_EXIT();

    return (ety != NULL) ? VTSS_RC_OK : VTSS_RC_ERROR;
}

/* Internal (Inter-Module) access management entry update */
mesa_rc access_mgmt_internal_entry_upd(const access_mgmt_inter_module_entry_t *const entry)
{
    u32                                 idx;
    access_mgmt_inter_module_entry_t    *ety;

    if (!entry) {
        return VTSS_RC_ERROR;
    }

    if (!ACCESS_MGMT_internal_entry_sanity(entry, TRUE)) {
        return VTSS_RC_ERROR;
    }

    ety = NULL;
    ACCESS_MGMT_CRIT_ENTER();
    for (idx = 0; idx < ACCESS_MGMT_MAX_ENTRIES; ++idx) {
        if (ACCESS_MGMT_global.access_mgmt_internal[idx].source == entry->source) {
            ety = &ACCESS_MGMT_global.access_mgmt_internal[idx];
            if (memcmp(ety, entry, sizeof(access_mgmt_inter_module_entry_t))) {
                memcpy(ety, entry, sizeof(access_mgmt_inter_module_entry_t));
                vtss_flag_setbits(&ACCESS_MGMT_thread_flag, ACCESS_MGMT_EVENT_POKE);
            }
            break;
        }
    }
    ACCESS_MGMT_CRIT_EXIT();

    return (ety != NULL) ? VTSS_RC_OK : VTSS_RC_ERROR;
}

/* Internal (Inter-Module) access management entry get */
mesa_rc access_mgmt_internal_entry_get(access_mgmt_inter_module_entry_t *const entry)
{
    u32                                 idx;
    access_mgmt_inter_module_entry_t    *ety;

    if (!entry) {
        return VTSS_RC_ERROR;
    }

    if (!ACCESS_MGMT_internal_entry_sanity(entry, FALSE)) {
        return VTSS_RC_ERROR;
    }

    ety = NULL;
    ACCESS_MGMT_CRIT_ENTER();
    for (idx = 0; idx < ACCESS_MGMT_MAX_ENTRIES; ++idx) {
        if (ACCESS_MGMT_global.access_mgmt_internal[idx].source == entry->source) {
            ety = &ACCESS_MGMT_global.access_mgmt_internal[idx];
            memcpy(entry, ety, sizeof(access_mgmt_inter_module_entry_t));
            break;
        }
    }
    ACCESS_MGMT_CRIT_EXIT();

    return (ety != NULL) ? VTSS_RC_OK : VTSS_RC_ERROR;
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void access_mgmt_mib_init(void);
#endif /* VTSS_SW_OPTION_PRIVATE_MIB */
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_access_mgmt_json_init(void);
#endif
extern "C" int access_mgmt_icli_cmd_register();

/* Initialize module */
mesa_rc access_mgmt_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;
    mesa_rc     rc = VTSS_RC_OK;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");
        /* Create semaphore for critical regions */
        critd_init(&ACCESS_MGMT_global.crit, "access_mgmt", VTSS_MODULE_ID_ACCESS_MGMT, CRITD_TYPE_MUTEX);

#ifdef VTSS_SW_OPTION_ICFG
        rc = access_mgmt_icfg_init();
        if (rc != VTSS_RC_OK) {
            T_D("fail to init icfg registration, rc = %s", error_txt(rc));
        }
#endif
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        access_mgmt_mib_init();  /* Register Access Management private mib */
#endif /* VTSS_SW_OPTION_PRIVATE_MIB */
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_access_mgmt_json_init();
#endif
        access_mgmt_icli_cmd_register();
        break;

    case INIT_CMD_START:
        ACCESS_MGMT_initialize();
        T_D("START");
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
        } else if (isid == VTSS_ISID_GLOBAL) {
            /* Reset stack configuration */
            ACCESS_MGMT_conf_reset();
        } else if (VTSS_ISID_LEGAL(isid)) {
            /* Reset switch configuration */
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");
        access_mgmt_stats_clear();
        rc = ACCESS_MGMT_start();
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_D("ICFG_LOADING_POST, isid: %d", isid);
        /* Apply all configuration to switch */
        rc = ACCESS_MGMT_conf_apply(isid);
        break;

    default:
        break;
    }

    return rc;
}

/* Get access management entry */
mesa_rc access_mgmt_entry_get(int access_id, access_mgmt_entry_t *entry)
{
    access_mgmt_conf_t conf;

    T_D("enter");

    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return ACCESS_MGMT_ERROR_STACK_STATE;
    }

    /* Check illegal parameter */
    if (access_id < ACCESS_MGMT_ACCESS_ID_START || access_id >= ACCESS_MGMT_ACCESS_ID_START + ACCESS_MGMT_MAX_ENTRIES) {
        T_D("exit");
        return ACCESS_MGMT_ERROR_PARM;
    }

    VTSS_RC(access_mgmt_conf_get(&conf));
    *entry = conf.entry[access_id];

    T_D("exit");
    return VTSS_RC_OK;
}

/* Add access management entry
   valid access_id: ACCESS_MGMT_ACCESS_ID_START ~ ACCESS_MGMT_MAX_ENTRIES */
mesa_rc access_mgmt_entry_add(int access_id, access_mgmt_entry_t *entry)
{
    mesa_rc rc;
    access_mgmt_conf_t conf;

    T_D("enter");

    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return ACCESS_MGMT_ERROR_STACK_STATE;
    }

    /* Check illegal parameter */
    if (access_id < ACCESS_MGMT_ACCESS_ID_START || access_id >= ACCESS_MGMT_ACCESS_ID_START + ACCESS_MGMT_MAX_ENTRIES) {
        T_D("exit");
        return ACCESS_MGMT_ERROR_PARM;
    }
    if (entry->service_type > ACCESS_MGMT_SERVICES_TYPE ||
        entry->entry_type > ACCESS_MGMT_ENTRY_TYPE_IPV6 ||
        entry->vid < 1 || entry->vid > VTSS_APPL_VLAN_ID_MAX) {
        T_D("exit");
        return ACCESS_MGMT_ERROR_PARM;
    }

    if (entry->service_type == 0 || entry->service_type & (~ACCESS_MGMT_SERVICES_TYPE)) {
        T_D("exit");
        return ACCESS_MGMT_ERROR_PARM;
    }

    if (entry->entry_type != ACCESS_MGMT_ENTRY_TYPE_IPV4 && entry->entry_type != ACCESS_MGMT_ENTRY_TYPE_IPV6) {
        return ACCESS_MGMT_ERROR_PARM;
    }

    if (entry->entry_type == ACCESS_MGMT_ENTRY_TYPE_IPV4) {
        if (entry->start_ip > entry->end_ip) {
            T_D("exit");
            return ACCESS_MGMT_ERROR_PARM;
        }
    }
#ifdef VTSS_SW_OPTION_IPV6
    else if (entry->entry_type == ACCESS_MGMT_ENTRY_TYPE_IPV6) {
        if (memcmp(&entry->start_ipv6, &entry->end_ipv6, sizeof(mesa_ipv6_t)) > 0) {
            T_D("exit");
            return ACCESS_MGMT_ERROR_PARM;
        }
    }
#endif /* VTSS_SW_OPTION_IPV6 */
    else {
        T_D("exit");
        return ACCESS_MGMT_ERROR_PARM;
    }

    if ((rc = access_mgmt_conf_get(&conf)) != VTSS_RC_OK) {
        T_D("exit");
        return rc;
    }

    if (!conf.entry[access_id].valid) {
        conf.entry_num++;
    }

    conf.entry[access_id] = *entry;
    conf.entry[access_id].valid = 1;
    rc = access_mgmt_conf_set(&conf);

    T_D("exit");
    return rc;
}

/* Delete access management entry */
mesa_rc access_mgmt_entry_del(int access_id)
{
    mesa_rc rc;
    access_mgmt_conf_t conf;

    T_D("enter");

    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return ACCESS_MGMT_ERROR_STACK_STATE;
    }

    /* Check illegal parameter */
    if (access_id < ACCESS_MGMT_ACCESS_ID_START || access_id >= ACCESS_MGMT_ACCESS_ID_START + ACCESS_MGMT_MAX_ENTRIES) {
        T_D("exit");
        return ACCESS_MGMT_ERROR_PARM;
    }

    if ((rc = access_mgmt_conf_get(&conf)) != VTSS_RC_OK) {
        T_D("exit");
        return rc;
    }

    if (conf.entry_num > 0) {
        conf.entry_num--;
    }

    memset(&conf.entry[access_id], 0x0, sizeof(conf.entry[access_id]));

    rc = access_mgmt_conf_set(&conf);

    T_D("exit");
    return rc;
}


/* Clear access management entry */
mesa_rc access_mgmt_entry_clear(void)
{
    mesa_rc             rc;
    u32                 curr_mode;
    access_mgmt_conf_t  conf;

    T_D("enter");

    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return ACCESS_MGMT_ERROR_STACK_STATE;
    }

    if ((rc = access_mgmt_conf_get(&conf)) != VTSS_RC_OK) {
        T_D("exit");
        return rc;
    }

    curr_mode = conf.mode;
    memset(&conf, 0x0, sizeof(conf));
    conf.mode = curr_mode;
    rc = access_mgmt_conf_set(&conf);

    T_D("exit");
    return rc;
}

/* Get access management statistics */
void access_mgmt_stats_get(access_mgmt_stats_t *stats)
{
    T_D("enter");

    ACCESS_MGMT_CRIT_ENTER();

    stats->http_receive_cnt     = ACCESS_MGMT_http_receive_cnt;
    stats->http_discard_cnt     = ACCESS_MGMT_http_discard_cnt;
    stats->https_receive_cnt    = ACCESS_MGMT_https_receive_cnt;
    stats->https_discard_cnt    = ACCESS_MGMT_https_discard_cnt;
    stats->snmp_receive_cnt     = ACCESS_MGMT_snmp_receive_cnt;
    stats->snmp_discard_cnt     = ACCESS_MGMT_snmp_discard_cnt;
    stats->telnet_receive_cnt   = ACCESS_MGMT_telnet_receive_cnt;
    stats->telnet_discard_cnt   = ACCESS_MGMT_telnet_discard_cnt;
    stats->ssh_receive_cnt      = ACCESS_MGMT_ssh_receive_cnt;
    stats->ssh_discard_cnt      = ACCESS_MGMT_ssh_discard_cnt;

    ACCESS_MGMT_CRIT_EXIT();

    T_D("exit");
}

/* Clear access management statistics */
void access_mgmt_stats_clear(void)
{
    T_D("enter");

    ACCESS_MGMT_CRIT_ENTER();

    ACCESS_MGMT_http_receive_cnt    = 0;
    ACCESS_MGMT_http_discard_cnt    = 0;
    ACCESS_MGMT_https_receive_cnt   = 0;
    ACCESS_MGMT_https_discard_cnt   = 0;
    ACCESS_MGMT_snmp_receive_cnt    = 0;
    ACCESS_MGMT_snmp_discard_cnt    = 0;
    ACCESS_MGMT_telnet_receive_cnt  = 0;
    ACCESS_MGMT_telnet_discard_cnt  = 0;
    ACCESS_MGMT_ssh_receive_cnt     = 0;
    ACCESS_MGMT_ssh_discard_cnt     = 0;

    ACCESS_MGMT_CRIT_EXIT();

    T_D("exit");
}

/* Check if entry content is the same as others
   Retrun: 0 - no duplicated, others - duplicated access_id */
int access_mgmt_entry_content_is_duplicated(int access_id, access_mgmt_entry_t *entry)
{
    access_mgmt_conf_t *conf;
    int access_idx, found = 0;

    ACCESS_MGMT_CRIT_ENTER();
    conf = &ACCESS_MGMT_global.access_mgmt_conf;
    for (access_idx = ACCESS_MGMT_ACCESS_ID_START; access_idx < ACCESS_MGMT_ACCESS_ID_START + ACCESS_MGMT_MAX_ENTRIES; access_idx++) {
        if (access_idx == access_id || !conf->entry[access_idx].valid) {
            continue;
        }

        if (conf->entry[access_idx].entry_type == ACCESS_MGMT_ENTRY_TYPE_IPV4) {
            if ((conf->entry[access_idx].start_ip == entry->start_ip) &&
                (conf->entry[access_idx].end_ip == entry->end_ip)) {
                found = access_idx;
                break;
            }
#ifdef VTSS_SW_OPTION_IPV6
        } else {
            if (!memcmp(&conf->entry[access_idx].start_ipv6, &entry->start_ipv6, sizeof(mesa_ipv6_t)) &&
                !memcmp(&conf->entry[access_idx].end_ipv6, &entry->end_ipv6, sizeof(mesa_ipv6_t))) {
                found = access_idx;
                break;
            }
#endif /* VTSS_SW_OPTION_IPV6 */
        }
    }
    ACCESS_MGMT_CRIT_EXIT();

    if (found) {
        return access_idx;
    }
    return 0;
}

/*****************************************************************************
    Public API section for Access Management
    from vtss_appl/include/vtss/appl/access_management.h
*****************************************************************************/
static mesa_rc
_vtss_appl_access_mgmt_table_itr(
    const u32                           *const prev,
    u32                                 *const next,
    BOOL                                ipv6_only
)
{
    u32                 nxt;
    access_mgmt_entry_t entry;

    if (!next) {
        return VTSS_RC_ERROR;
    }

    if (prev) {
        nxt = *prev + 1;
        if (nxt > ACCESS_MGMT_MAX_ENTRIES) {
            T_I("end with PREV=%u(NXT=%u)", *prev, nxt);
            return VTSS_RC_ERROR;
        }
    } else {
        nxt = ACCESS_MGMT_ACCESS_ID_START;
    }

    T_I("NxtStart: %u", nxt);
    for (; !(nxt > ACCESS_MGMT_MAX_ENTRIES); nxt++) {
        if (access_mgmt_entry_get((int)nxt, &entry) == VTSS_RC_OK && entry.valid) {
            if ((entry.entry_type == ACCESS_MGMT_ENTRY_TYPE_IPV6) == ipv6_only) {
                T_I("found(Type%s): NXT=%u/%s",
                    ipv6_only ? "V6" : "V4",
                    nxt,
                    (entry.entry_type == ACCESS_MGMT_ENTRY_TYPE_IPV6) ? "IPv6" : "IPv4");
                *next = nxt;
                return VTSS_RC_OK;
            }
        }
    }

    return VTSS_RC_ERROR;
}

/**
 * \brief Get Access Management Parameters
 *
 * To read current system parameters in Access Management.
 *
 * \param conf      [OUT]    The Access Management system configuration data.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_system_config_get(
    vtss_appl_access_mgmt_conf_t        *const conf
)
{
    access_mgmt_conf_t  globals;
    mesa_rc             rc = VTSS_RC_ERROR;

    if (!conf) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    if ((rc = access_mgmt_conf_get(&globals)) == VTSS_RC_OK) {
        conf->mode = (globals.mode == ACCESS_MGMT_ENABLED);
    }

    return rc;
}

/**
 * \brief Set Access Management Parameters
 *
 * To modify current system parameters in Access Management.
 *
 * \param conf      [IN]     The Access Management system configuration data.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_system_config_set(
    const vtss_appl_access_mgmt_conf_t  *const conf
)
{
    access_mgmt_conf_t  globals;
    mesa_rc             rc = VTSS_RC_ERROR;

    if (!conf) {
        T_D("Invalid Input!");
        return rc;
    }

    if ((rc = access_mgmt_conf_get(&globals)) == VTSS_RC_OK) {
        globals.mode = conf->mode ? ACCESS_MGMT_ENABLED : ACCESS_MGMT_DISABLED;
        rc = access_mgmt_conf_set(&globals);
    }

    return rc;
}

/**
 * \brief Access Management Control ACTION
 *
 * Action flag to denote clearing Access Management statistics.
 * This flag is active only when SET is involved and its value is set to be TRUE.
 * When it is active, it means we should take action for clearing all the counters
 * in Access Management statistics table.
 *
 * \param clr_flag  [IN]    Clear Access Management statistics action to be taken.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_control_statistics_clr(
    const BOOL                          *clr_flag
)
{
    if (clr_flag && *clr_flag) {
        access_mgmt_stats_clear();
    }

    return VTSS_RC_OK;
}

/**
 * \brief Iterator for retrieving Access Management IPv4 table key/index
 *
 * To walk access index of the IPv4 table in Access Management.
 *
 * \param prev      [IN]    Access index to be used for indexing determination.
 *
 * \param next      [OUT]   The key/index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_ipv4_table_itr(
    const u32                           *const prev,
    u32                                 *const next
)
{
    mesa_rc rc = VTSS_RC_ERROR;

    if (!next) {
        T_D("Invalid Input!");
        return rc;
    }

    T_D("enter: PREV=%d", prev ? *prev : -1);
    rc = _vtss_appl_access_mgmt_table_itr(prev, next, FALSE);
    T_D("exit(%s): NEXT=%u", rc == VTSS_RC_OK ? "OK" : "NG", *next);

    return rc;
}

/**
 * \brief Get Access Management specific access index configuration
 *
 * To read configuration of the specific access index in Access Management IPv4 table.
 *
 * \param access_id [IN]    (key) Access index.  Accepted values are in the range [1; 16].
 * \param ipv4_conf [OUT]   The current configuration of the access index.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_ipv4_table_get(
    u32                                 access_id,
    vtss_appl_access_mgmt_ipv4_t        *const ipv4_conf
)
{
    access_mgmt_entry_t entry;

    if (!ipv4_conf) {
        T_D("Invalid Input!");
        return ACCESS_MGMT_ERROR_NULL_PARM;
    }

    VTSS_RC(access_mgmt_entry_get(access_id, &entry));

    if (entry.valid && entry.entry_type == ACCESS_MGMT_ENTRY_TYPE_IPV4) {
        ipv4_conf->vlan_id = entry.vid;

#if defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
        if (entry.service_type & ACCESS_MGMT_SERVICES_TYPE_WEB) {
            ipv4_conf->web_services = TRUE;
        } else {
            ipv4_conf->web_services = FALSE;
        }
#endif /* VTSS_SW_OPTION_WEB  || VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI */
#ifdef VTSS_SW_OPTION_SNMP
        if (entry.service_type & ACCESS_MGMT_SERVICES_TYPE_SNMP) {
            ipv4_conf->snmp_services = TRUE;
        } else {
            ipv4_conf->snmp_services = FALSE;
        }
#endif /* VTSS_SW_OPTION_SNMP */
        if (entry.service_type & ACCESS_MGMT_SERVICES_TYPE_TELNET) {
            ipv4_conf->telnet_services = TRUE;
        } else {
            ipv4_conf->telnet_services = FALSE;
        }

        ipv4_conf->start_address = entry.start_ip;
        ipv4_conf->end_address = entry.end_ip;
    } else {
        return ACCESS_MGMT_ERROR_NOT_IPV4_INPUT_TYPE;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Set Access Management specific access index configuration
 *
 * To modify configuration of the specific access index in Access Management IPv4 table.
 *
 * \param access_id [IN]    (key) Access index.  Accepted values are in the range [1; 16].
 * \param ipv4_conf [IN]    The revised configuration of the access index.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_ipv4_table_set(
    u32                                 access_id,
    const vtss_appl_access_mgmt_ipv4_t  *const ipv4_conf
)
{
    access_mgmt_entry_t entry;
    int                 web_services = -2;
    int                 snmp_services = -2;
    int                 shell_services = -2;

    if (!ipv4_conf) {
        T_D("Invalid Input!");
        return ACCESS_MGMT_ERROR_NULL_PARM;
    }

#if defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
    web_services = ipv4_conf->web_services ? 1 : 0;
#else
    web_services = -1;
#endif /* VTSS_SW_OPTION_WEB  || VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI */
#ifdef VTSS_SW_OPTION_SNMP
    snmp_services = ipv4_conf->snmp_services ? 1 : 0;
#else
    snmp_services = -1;
#endif /* VTSS_SW_OPTION_SNMP */
    shell_services = ipv4_conf->telnet_services ? 1 : 0;

    T_D("SET AccessID:%u with %sWEB/%sSNMP/%sSHELL/%u/%u/%u",
        access_id,
        web_services == 0 ? "No" : (web_services > 0 ? "" : "x"),
        snmp_services == 0 ? "No" : (snmp_services > 0 ? "" : "x"),
        shell_services == 0 ? "No" : (shell_services > 0 ? "" : "x"),
        ipv4_conf->vlan_id,
        ipv4_conf->start_address,
        ipv4_conf->end_address);

    VTSS_RC(access_mgmt_entry_get(access_id, &entry));

    if (entry.valid && entry.entry_type == ACCESS_MGMT_ENTRY_TYPE_IPV4) {
        entry.service_type = 0;
#if defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
        if (ipv4_conf->web_services) {
            T_I("Set WEB with SVC-TYP:%u", entry.service_type);
            entry.service_type |= ACCESS_MGMT_SERVICES_TYPE_WEB;
        }
#endif /* VTSS_SW_OPTION_WEB  || VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI */
#ifdef VTSS_SW_OPTION_SNMP
        if (ipv4_conf->snmp_services) {
            T_I("Set SNMP with SVC-TYP:%u", entry.service_type);
            entry.service_type |= ACCESS_MGMT_SERVICES_TYPE_SNMP;
        }
#endif /* VTSS_SW_OPTION_SNMP */
        if (ipv4_conf->telnet_services) {
            T_I("Set SHELL with SVC-TYP:%u", entry.service_type);
            entry.service_type |= ACCESS_MGMT_SERVICES_TYPE_TELNET;
        }

        if (entry.service_type & ACCESS_MGMT_SERVICES_TYPE) {
            entry.vid = ipv4_conf->vlan_id;
            entry.start_ip = ipv4_conf->start_address;
            entry.end_ip = ipv4_conf->end_address;

            if (access_mgmt_entry_content_is_duplicated(access_id, &entry)) {
                T_D("Duplicate entry content!");
                return ACCESS_MGMT_ERROR_DUPLICATED_CONTENT;
            } else {
                VTSS_RC(access_mgmt_entry_add(access_id, &entry));
            }
        } else {
            T_D("Invalid entry content (SVC-TYP:%u)!", entry.service_type);
            return ACCESS_MGMT_ERROR_INVALID_SERVICE;
        }
    } else {
        T_D("Set invalid entry!");
        return ACCESS_MGMT_ERROR_NOT_IPV4_INPUT_TYPE;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Delete Access Management specific access index configuration
 *
 * To delete configuration of the specific access index in Access Management IPv4 table.
 *
 * \param access_id [IN]    (key) Access index.  Accepted values are in the range [1; 16].
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_ipv4_table_del(
    u32                                 access_id
)
{
    access_mgmt_entry_t entry;
    mesa_rc             rc = VTSS_RC_ERROR;

    T_D("DEL AccessID:%u", access_id);
    if ((rc = access_mgmt_entry_get(access_id, &entry)) == VTSS_RC_OK) {
        if (entry.valid && entry.entry_type == ACCESS_MGMT_ENTRY_TYPE_IPV4) {
            rc = access_mgmt_entry_del(access_id);
        } else {
            T_D("Delete invalid entry!");
            rc = VTSS_RC_ERROR;
        }
    } else {
        T_D("Delete non-existing entry!");
        rc = VTSS_RC_ERROR;
    }
    T_D("DEL Done:%d", rc);

    return rc;
}

/**
 * \brief Add Access Management specific access index configuration
 *
 * To add configuration of the specific access index in Access Management IPv4 table.
 *
 * \param access_id [IN]    (key) Access index.  Accepted values are in the range [1; 16].
 * \param ipv4_conf [IN]    The new configuration of the access index.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_ipv4_table_add(
    u32                                 access_id,
    const vtss_appl_access_mgmt_ipv4_t  *const ipv4_conf
)
{
    access_mgmt_entry_t entry;
    mesa_rc             rc = VTSS_RC_ERROR;
    int                 web_services = -2;
    int                 snmp_services = -2;
    int                 shell_services = -2;

    if (!ipv4_conf) {
        T_D("Invalid Input!");
        return rc;
    }

#if defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
    web_services = ipv4_conf->web_services ? 1 : 0;
#else
    web_services = -1;
#endif /* VTSS_SW_OPTION_WEB  || VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI */
#ifdef VTSS_SW_OPTION_SNMP
    snmp_services = ipv4_conf->snmp_services ? 1 : 0;
#else
    snmp_services = -1;
#endif /* VTSS_SW_OPTION_SNMP */
    shell_services = ipv4_conf->telnet_services ? 1 : 0;

    T_D("ADD AccessID:%u with %sWEB/%sSNMP/%sSHELL/%u/%u/%u",
        access_id,
        web_services == 0 ? "No" : (web_services > 0 ? "" : "x"),
        snmp_services == 0 ? "No" : (snmp_services > 0 ? "" : "x"),
        shell_services == 0 ? "No" : (shell_services > 0 ? "" : "x"),
        ipv4_conf->vlan_id,
        ipv4_conf->start_address,
        ipv4_conf->end_address);
    if (((rc = access_mgmt_entry_get(access_id, &entry)) != VTSS_RC_OK) ||
        !entry.valid) {
        memset(&entry, 0x0, sizeof(access_mgmt_entry_t));

        entry.entry_type = ACCESS_MGMT_ENTRY_TYPE_IPV4;
#if defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
        if (ipv4_conf->web_services) {
            T_I("Set WEB with SVC-TYP:%u", entry.service_type);
            entry.service_type |= ACCESS_MGMT_SERVICES_TYPE_WEB;
        }
#endif /* VTSS_SW_OPTION_WEB  || VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI */
#ifdef VTSS_SW_OPTION_SNMP
        if (ipv4_conf->snmp_services) {
            T_I("Set SNMP with SVC-TYP:%u", entry.service_type);
            entry.service_type |= ACCESS_MGMT_SERVICES_TYPE_SNMP;
        }
#endif /* VTSS_SW_OPTION_SNMP */
        if (ipv4_conf->telnet_services) {
            T_I("Set SHELL with SVC-TYP:%u", entry.service_type);
            entry.service_type |= ACCESS_MGMT_SERVICES_TYPE_TELNET;
        }

        if (entry.service_type & ACCESS_MGMT_SERVICES_TYPE) {
            entry.vid = ipv4_conf->vlan_id;
            entry.start_ip = ipv4_conf->start_address;
            entry.end_ip = ipv4_conf->end_address;

            if (access_mgmt_entry_content_is_duplicated(access_id, &entry)) {
                T_D("Duplicate entry content!");
                rc = VTSS_RC_ERROR;
            } else {
                rc = access_mgmt_entry_add(access_id, &entry);
            }
        } else {
            T_D("Invalid entry content (SVC-TYP:%u)!", entry.service_type);
            rc = VTSS_RC_ERROR;
        }
    } else {
        T_D("Add existing entry!");
        rc = VTSS_RC_ERROR;
    }
    T_D("ADD Done:%d", rc);

    return rc;
}

#ifdef VTSS_SW_OPTION_IPV6
/**
 * \brief Iterator for retrieving Access Management IPv6 table key/index
 *
 * To walk access index of the IPv6 table in Access Management.
 *
 * \param prev      [IN]    Access index to be used for indexing determination.
 *
 * \param next      [OUT]   The key/index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_ipv6_table_itr(
    const u32                           *const prev,
    u32                                 *const next
)
{
    mesa_rc rc = VTSS_RC_ERROR;

    if (!next) {
        T_D("Invalid Input!");
        return rc;
    }

    T_D("enter: PREV=%d", prev ? *prev : -1);
    rc = _vtss_appl_access_mgmt_table_itr(prev, next, TRUE);
    T_D("exit(%s): NEXT=%u", rc == VTSS_RC_OK ? "OK" : "NG", *next);

    return rc;
}

/**
 * \brief Get Access Management specific access index configuration
 *
 * To read configuration of the specific access index in Access Management IPv6 table.
 *
 * \param access_id [IN]    (key) Access index.  Accepted values are in the range [1; 16].
 * \param ipv6_conf [OUT]   The current configuration of the access index.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_ipv6_table_get(
    u32                                 access_id,
    vtss_appl_access_mgmt_ipv6_t        *const ipv6_conf
)
{
    access_mgmt_entry_t entry;
    mesa_rc             rc = VTSS_RC_ERROR;

    if (!ipv6_conf) {
        T_D("Invalid Input!");
        return rc;
    }

    if ((rc = access_mgmt_entry_get(access_id, &entry)) == VTSS_RC_OK) {
        if (entry.valid && entry.entry_type == ACCESS_MGMT_ENTRY_TYPE_IPV6) {
            ipv6_conf->vlan_id = entry.vid;

#if defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
            if (entry.service_type & ACCESS_MGMT_SERVICES_TYPE_WEB) {
                ipv6_conf->web_services = TRUE;
            } else {
                ipv6_conf->web_services = FALSE;
            }
#endif /* VTSS_SW_OPTION_WEB  || VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI */
#ifdef VTSS_SW_OPTION_SNMP
            if (entry.service_type & ACCESS_MGMT_SERVICES_TYPE_SNMP) {
                ipv6_conf->snmp_services = TRUE;
            } else {
                ipv6_conf->snmp_services = FALSE;
            }
#endif /* VTSS_SW_OPTION_SNMP */
            if (entry.service_type & ACCESS_MGMT_SERVICES_TYPE_TELNET) {
                ipv6_conf->telnet_services = TRUE;
            } else {
                ipv6_conf->telnet_services = FALSE;
            }

            memcpy(&ipv6_conf->start_address, &entry.start_ipv6, sizeof(mesa_ipv6_t));
            memcpy(&ipv6_conf->end_address, &entry.end_ipv6, sizeof(mesa_ipv6_t));
        } else {
            rc = VTSS_RC_ERROR;
        }
    }
    T_D("GET AccessID:%u with RC=%d", access_id, rc);

    return rc;
}

/**
 * \brief Set Access Management specific access index configuration
 *
 * To modify configuration of the specific access index in Access Management IPv6 table.
 *
 * \param access_id [IN]    (key) Access index.  Accepted values are in the range [1; 16].
 * \param ipv6_conf [IN]    The revised configuration of the access index.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_ipv6_table_set(
    u32                                 access_id,
    const vtss_appl_access_mgmt_ipv6_t  *const ipv6_conf
)
{
    access_mgmt_entry_t entry;
    mesa_rc             rc = VTSS_RC_ERROR;
    int                 web_services = -2;
    int                 snmp_services = -2;
    int                 shell_services = -2;

    if (!ipv6_conf) {
        T_D("Invalid Input!");
        return rc;
    }

#if defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
    web_services = ipv6_conf->web_services ? 1 : 0;
#else
    web_services = -1;
#endif /* VTSS_SW_OPTION_WEB  || VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI */
#ifdef VTSS_SW_OPTION_SNMP
    snmp_services = ipv6_conf->snmp_services ? 1 : 0;
#else
    snmp_services = -1;
#endif /* VTSS_SW_OPTION_SNMP */
    shell_services = ipv6_conf->telnet_services ? 1 : 0;
    T_D("ADD AccessID:%u with %sWEB/%sSNMP/%sSHELL/%u/%u-%u:%u-%u:%u-%u:%u-%u:%u-%u:%u-%u:%u-%u:%u-%u/%u-%u:%u-%u:%u-%u:%u-%u:%u-%u:%u-%u:%u-%u:%u-%u",
        access_id,
        web_services == 0 ? "No" : (web_services > 0 ? "" : "x"),
        snmp_services == 0 ? "No" : (snmp_services > 0 ? "" : "x"),
        shell_services == 0 ? "No" : (shell_services > 0 ? "" : "x"),
        ipv6_conf->vlan_id,
        ipv6_conf->start_address.addr[0],
        ipv6_conf->start_address.addr[1],
        ipv6_conf->start_address.addr[2],
        ipv6_conf->start_address.addr[3],
        ipv6_conf->start_address.addr[4],
        ipv6_conf->start_address.addr[5],
        ipv6_conf->start_address.addr[6],
        ipv6_conf->start_address.addr[7],
        ipv6_conf->start_address.addr[8],
        ipv6_conf->start_address.addr[9],
        ipv6_conf->start_address.addr[10],
        ipv6_conf->start_address.addr[11],
        ipv6_conf->start_address.addr[12],
        ipv6_conf->start_address.addr[13],
        ipv6_conf->start_address.addr[14],
        ipv6_conf->start_address.addr[15],
        ipv6_conf->end_address.addr[0],
        ipv6_conf->end_address.addr[1],
        ipv6_conf->end_address.addr[2],
        ipv6_conf->end_address.addr[3],
        ipv6_conf->end_address.addr[4],
        ipv6_conf->end_address.addr[5],
        ipv6_conf->end_address.addr[6],
        ipv6_conf->end_address.addr[7],
        ipv6_conf->end_address.addr[8],
        ipv6_conf->end_address.addr[9],
        ipv6_conf->end_address.addr[10],
        ipv6_conf->end_address.addr[11],
        ipv6_conf->end_address.addr[12],
        ipv6_conf->end_address.addr[13],
        ipv6_conf->end_address.addr[14],
        ipv6_conf->end_address.addr[15]);
    if ((rc = access_mgmt_entry_get(access_id, &entry)) == VTSS_RC_OK) {
        if (entry.valid && entry.entry_type == ACCESS_MGMT_ENTRY_TYPE_IPV6) {
            entry.service_type = 0;
#if defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
            if (ipv6_conf->web_services) {
                T_I("Set WEB with SVC-TYP:%u", entry.service_type);
                entry.service_type |= ACCESS_MGMT_SERVICES_TYPE_WEB;
            }
#endif /* VTSS_SW_OPTION_WEB  || VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI */
#ifdef VTSS_SW_OPTION_SNMP
            if (ipv6_conf->snmp_services) {
                T_I("Set SNMP with SVC-TYP:%u", entry.service_type);
                entry.service_type |= ACCESS_MGMT_SERVICES_TYPE_SNMP;
            }
#endif /* VTSS_SW_OPTION_SNMP */
            if (ipv6_conf->telnet_services) {
                T_I("Set SHELL with SVC-TYP:%u", entry.service_type);
                entry.service_type |= ACCESS_MGMT_SERVICES_TYPE_TELNET;
            }

            if (entry.service_type & ACCESS_MGMT_SERVICES_TYPE) {
                entry.vid = ipv6_conf->vlan_id;
                memcpy(&entry.start_ipv6, &ipv6_conf->start_address, sizeof(mesa_ipv6_t));
                memcpy(&entry.end_ipv6, &ipv6_conf->end_address, sizeof(mesa_ipv6_t));

                if (access_mgmt_entry_content_is_duplicated(access_id, &entry)) {
                    T_D("Duplicate entry content!");
                    rc = VTSS_RC_ERROR;
                } else {
                    rc = access_mgmt_entry_add(access_id, &entry);
                }
            } else {
                T_D("Invalid entry content (SVC-TYP:%u)!", entry.service_type);
                rc = VTSS_RC_ERROR;
            }
        } else {
            T_D("Set invalid entry!");
            rc = VTSS_RC_ERROR;
        }
    } else {
        T_D("Set non-existing entry!");
        rc = VTSS_RC_ERROR;
    }
    T_D("SET Done:%d", rc);

    return rc;
}

/**
 * \brief Delete Access Management specific access index configuration
 *
 * To delete configuration of the specific access index in Access Management IPv6 table.
 *
 * \param access_id [IN]    (key) Access index.  Accepted values are in the range [1; 16].
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_ipv6_table_del(
    u32                                 access_id
)
{
    access_mgmt_entry_t entry;
    mesa_rc             rc = VTSS_RC_ERROR;

    T_D("DEL AccessID:%u", access_id);
    if ((rc = access_mgmt_entry_get(access_id, &entry)) == VTSS_RC_OK) {
        if (entry.valid && entry.entry_type == ACCESS_MGMT_ENTRY_TYPE_IPV6) {
            rc = access_mgmt_entry_del(access_id);
        } else {
            T_D("Delete invalid entry!");
            rc = VTSS_RC_ERROR;
        }
    } else {
        T_D("Delete non-existing entry!");
        rc = VTSS_RC_ERROR;
    }
    T_D("DEL Done:%d", rc);

    return rc;
}

/**
 * \brief Add Access Management specific access index configuration
 *
 * To add configuration of the specific access index in Access Management IPv6 table.
 *
 * \param access_id [IN]    (key) Access index.  Accepted values are in the range [1; 16].
 * \param ipv6_conf [IN]    The new configuration of the access index.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_ipv6_table_add(
    u32                                 access_id,
    const vtss_appl_access_mgmt_ipv6_t  *const ipv6_conf
)
{
    access_mgmt_entry_t entry;
    mesa_rc             rc = VTSS_RC_ERROR;
    int                 web_services = -2;
    int                 snmp_services = -2;
    int                 shell_services = -2;

    if (!ipv6_conf) {
        T_D("Invalid Input!");
        return rc;
    }

#if defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
    web_services = ipv6_conf->web_services ? 1 : 0;
#else
    web_services = -1;
#endif /* VTSS_SW_OPTION_WEB  || VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI */
#ifdef VTSS_SW_OPTION_SNMP
    snmp_services = ipv6_conf->snmp_services ? 1 : 0;
#else
    snmp_services = -1;
#endif /* VTSS_SW_OPTION_SNMP */
    shell_services = ipv6_conf->telnet_services ? 1 : 0;
    T_D("ADD AccessID:%u with %sWEB/%sSNMP/%sSHELL/%u/%u-%u:%u-%u:%u-%u:%u-%u:%u-%u:%u-%u:%u-%u:%u-%u/%u-%u:%u-%u:%u-%u:%u-%u:%u-%u:%u-%u:%u-%u:%u-%u",
        access_id,
        web_services == 0 ? "No" : (web_services > 0 ? "" : "x"),
        snmp_services == 0 ? "No" : (snmp_services > 0 ? "" : "x"),
        shell_services == 0 ? "No" : (shell_services > 0 ? "" : "x"),
        ipv6_conf->vlan_id,
        ipv6_conf->start_address.addr[0],
        ipv6_conf->start_address.addr[1],
        ipv6_conf->start_address.addr[2],
        ipv6_conf->start_address.addr[3],
        ipv6_conf->start_address.addr[4],
        ipv6_conf->start_address.addr[5],
        ipv6_conf->start_address.addr[6],
        ipv6_conf->start_address.addr[7],
        ipv6_conf->start_address.addr[8],
        ipv6_conf->start_address.addr[9],
        ipv6_conf->start_address.addr[10],
        ipv6_conf->start_address.addr[11],
        ipv6_conf->start_address.addr[12],
        ipv6_conf->start_address.addr[13],
        ipv6_conf->start_address.addr[14],
        ipv6_conf->start_address.addr[15],
        ipv6_conf->end_address.addr[0],
        ipv6_conf->end_address.addr[1],
        ipv6_conf->end_address.addr[2],
        ipv6_conf->end_address.addr[3],
        ipv6_conf->end_address.addr[4],
        ipv6_conf->end_address.addr[5],
        ipv6_conf->end_address.addr[6],
        ipv6_conf->end_address.addr[7],
        ipv6_conf->end_address.addr[8],
        ipv6_conf->end_address.addr[9],
        ipv6_conf->end_address.addr[10],
        ipv6_conf->end_address.addr[11],
        ipv6_conf->end_address.addr[12],
        ipv6_conf->end_address.addr[13],
        ipv6_conf->end_address.addr[14],
        ipv6_conf->end_address.addr[15]);
    if (((rc = access_mgmt_entry_get(access_id, &entry)) != VTSS_RC_OK) ||
        !entry.valid) {
        memset(&entry, 0x0, sizeof(access_mgmt_entry_t));

        entry.entry_type = ACCESS_MGMT_ENTRY_TYPE_IPV6;
#if defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
        if (ipv6_conf->web_services) {
            T_I("Set WEB with SVC-TYP:%u", entry.service_type);
            entry.service_type |= ACCESS_MGMT_SERVICES_TYPE_WEB;
        }
#endif /* VTSS_SW_OPTION_WEB  || VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI */
#ifdef VTSS_SW_OPTION_SNMP
        if (ipv6_conf->snmp_services) {
            T_I("Set SNMP with SVC-TYP:%u", entry.service_type);
            entry.service_type |= ACCESS_MGMT_SERVICES_TYPE_SNMP;
        }
#endif /* VTSS_SW_OPTION_SNMP */
        if (ipv6_conf->telnet_services) {
            T_I("Set SHELL with SVC-TYP:%u", entry.service_type);
            entry.service_type |= ACCESS_MGMT_SERVICES_TYPE_TELNET;
        }

        if (entry.service_type & ACCESS_MGMT_SERVICES_TYPE) {
            entry.vid = ipv6_conf->vlan_id;
            memcpy(&entry.start_ipv6, &ipv6_conf->start_address, sizeof(mesa_ipv6_t));
            memcpy(&entry.end_ipv6, &ipv6_conf->end_address, sizeof(mesa_ipv6_t));

            if (access_mgmt_entry_content_is_duplicated(access_id, &entry)) {
                T_D("Duplicate entry content!");
                rc = VTSS_RC_ERROR;
            } else {
                rc = access_mgmt_entry_add(access_id, &entry);
            }
        } else {
            T_D("Invalid entry content (SVC-TYP:%u)!", entry.service_type);
            rc = VTSS_RC_ERROR;
        }
    } else {
        T_D("Add existing entry!");
        rc = VTSS_RC_ERROR;
    }
    T_D("ADD Done:%d", rc);

    return rc;
}
#endif /* VTSS_SW_OPTION_IPV6 */

/**
 * \brief Get current Access Management statistics
 *
 * To read the counters of received/allowed/discarded HTTP/HTTPS/SNMP/TELNET/SSH frames.
 *
 * \param pkt_cntr  [OUT]   The current packet counters for HTTP/HTTPS/SNMP/TELNET/SSH.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_statistics_get(
    vtss_appl_access_mgmt_statistics_t  *const pkt_cntr
)
{
    access_mgmt_stats_t stats;

    if (!pkt_cntr) {
        return VTSS_RC_ERROR;
    }

    access_mgmt_stats_get(&stats);
#if defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
    pkt_cntr->http_receive_cnt      = stats.http_receive_cnt;
    pkt_cntr->http_permit_cnt       = stats.http_receive_cnt - stats.http_discard_cnt;
    pkt_cntr->http_discard_cnt      = stats.http_discard_cnt;
    pkt_cntr->https_receive_cnt     = stats.https_receive_cnt;
    pkt_cntr->https_permit_cnt      = stats.https_receive_cnt - stats.https_discard_cnt;
    pkt_cntr->https_discard_cnt     = stats.https_discard_cnt;
#endif /* VTSS_SW_OPTION_WEB  || VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI */
#ifdef VTSS_SW_OPTION_SNMP
    pkt_cntr->snmp_receive_cnt      = stats.snmp_receive_cnt;
    pkt_cntr->snmp_permit_cnt       = stats.snmp_receive_cnt - stats.snmp_discard_cnt;
    pkt_cntr->snmp_discard_cnt      = stats.snmp_discard_cnt;
#endif /* VTSS_SW_OPTION_SNMP */
    pkt_cntr->telnet_receive_cnt    = stats.telnet_receive_cnt;
    pkt_cntr->telnet_permit_cnt     = stats.telnet_receive_cnt - stats.telnet_discard_cnt;
    pkt_cntr->telnet_discard_cnt    = stats.telnet_discard_cnt;
    pkt_cntr->ssh_receive_cnt       = stats.ssh_receive_cnt;
    pkt_cntr->ssh_permit_cnt        = stats.ssh_receive_cnt - stats.ssh_discard_cnt;
    pkt_cntr->ssh_discard_cnt       = stats.ssh_discard_cnt;

    return VTSS_RC_OK;
}

