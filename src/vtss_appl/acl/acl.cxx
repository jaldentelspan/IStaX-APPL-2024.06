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

#include "main.h"
#ifdef VTSS_SW_OPTION_PACKET
#include "packet_api.h"
#endif
#include "msg_api.h"        // msg_switch_exists(), msg_switch_configurable()
#include "port_api.h"       // port_vol_conf_t
#include "port_iter.hxx"
#include "vlan_api.h"       // VTSS_APPL_VLAN_ID_MAX
#include "critd_api.h"
#include "standalone_api.h" // topo_usid2isid(), topo_isid2usid()
#include "vtss_vcap_api.h"
#include "acl_api.h"
#include "acl.h"
#include "misc_api.h"

#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#endif

#ifdef VTSS_SW_OPTION_ICFG
#include "acl_icfg.h"
#endif

#ifdef VTSS_SW_OPTION_ICLI
#include "icli_porting_util.h" // For icli_port_info_txt()
#endif

#include <netinet/in.h>
#include <netinet/ip6.h>

#include "vtss_common_iterator.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-static-range.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-range.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-N.hxx"

/* JSON notification */
#include "vtss/basics/expose/table-status.hxx"  // For vtss::expose::TableStatus
#include "vtss/basics/memcmp-operator.hxx"      // For VTSS_BASICS_MEMCMP_OPERATOR

#if defined(VTSS_SW_OPTION_SNMP)
#include "ifIndex_api.h"
#endif

#include "acl_serializer.hxx"

#ifdef VTSS_SW_OPTION_IP
#include "ip_filter_api.hxx"
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_ACL

/* Define this for testing CPU packet reception performance */
#undef ACL_PACKET_TEST

const char *acl_user_txt[ACL_USER_CNT] = {
    "static",

#if defined(VTSS_SW_OPTION_IP_MGMT_ACL)
    "ipManagement",
#endif

#ifdef VTSS_SW_OPTION_IP_SOURCE_GUARD
    "ipSourceGuard",
#endif

#ifdef VTSS_SW_OPTION_IPV6_SOURCE_GUARD
    "ipv6SourceGuard",
#endif

#ifdef VTSS_SW_OPTION_IP
    "IP",
#endif

#if defined(VTSS_SW_OPTION_IPMC) || defined(VTSS_SW_OPTION_IGMPS) || defined(VTSS_SW_OPTION_MLDSNP)
    "ipmc",
#endif

#ifdef VTSS_SW_OPTION_CFM
    "Connectivity Fault Management",
#endif

#ifdef VTSS_SW_OPTION_APS
    "Automatic (Linear) Protection Switching",
#endif

#ifdef VTSS_SW_OPTION_ERPS
    "Ethernet Ring Protection Switching",
#endif

#ifdef VTSS_SW_OPTION_IEC_MRP
    "Media Redundancy Protocol",
#endif

#ifdef VTSS_SW_OPTION_REDBOX
    "Redbox",
#endif

#ifdef VTSS_SW_OPTION_ARP_INSPECTION
    "arpInspection",
#endif

#ifdef VTSS_SW_OPTION_UPNP
    "upnp",
#endif

#if defined(VTSS_SW_OPTION_PTP) || defined(VTSS_SW_OPTION_PHY_1588_SIM)
    "ptp",
#endif

#ifdef VTSS_SW_OPTION_DHCP_HELPER
    "dhcp",
#endif

#ifdef VTSS_SW_OPTION_DHCP6_SNOOPING
    "dhcp6Snooping",
#endif

#ifdef VTSS_SW_OPTION_LOOP_PROTECTION
    "loopProtect",
#endif

#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
    "linkOam",
#endif

#ifdef VTSS_SW_OPTION_ZTP
    "ztp",
#endif

    "test",
};

/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

/* Global structure */
static acl_global_t acl;

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "acl", "Access Control Lists"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define ACL_CRIT_ENTER() critd_enter(&acl.crit, __FILE__, __LINE__)
#define ACL_CRIT_EXIT()  critd_exit( &acl.crit, __FILE__, __LINE__)

struct ACL_Lock {
    ACL_Lock(const char *file, int line)
    {
        critd_enter(&acl.crit, file, line);
    }

    ~ACL_Lock()
    {
        critd_exit(&acl.crit, __FILE__, 0);
    }
};

#define ACL_LOCK_SCOPE() ACL_Lock __acl_lock_guard__(__FILE__, __LINE__)
#define ACL_LOCK_ASSERT_LOCKED(_fmt_, ...) if (!critd_is_locked(&acl.crit)) {T_E(_fmt_, ##__VA_ARGS__);}

static void acl_packet_register(void);
static void acl_local_ace_conflict_solve(BOOL new_volatile_ace_conflict);

/* JSON notification */
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_acl_status_ace_event_t);
vtss::expose::TableStatus <
vtss::expose::ParamKey<vtss_usid_t>,
     vtss::expose::ParamVal<vtss_appl_acl_status_ace_event_t *>
     > acl_status_ace_event_update("acl_status_ace_event_update", VTSS_MODULE_ID_ACL);

using namespace vtss;
#ifdef VTSS_SW_OPTION_IP
using namespace vtss::appl::ip::filter;
#endif

/* For the API of ACL bitmask, it maintains 4 offsets, the valid value is from 0 to 3 */
#define ACL_L26_ACE_CNT                     256
#define ACL_BITMASK_MAX_OFFSET              3
#define ACL_BITMASK_BITS_CNT                128
#define ACL_BITMASK_ARRAY_SIZE              (((ACL_L26_ACE_CNT) / ACL_BITMASK_BITS_CNT) + ((ACL_L26_ACE_CNT) % ACL_BITMASK_BITS_CNT))
#define ACL_BITMASK_ARRAY_OFFSET(ace_id)    ((ace_id) / ACL_BITMASK_BITS_CNT)

#ifdef VTSS_SW_OPTION_IP
/* Global database for saving ACL ifmux */
struct ace_ifmux_t {
    Owner                   owner = { .module_id = VTSS_MODULE_ID_ACL, .name = "ACL" }; /* Interface mux owner */
    Vector<mesa_ace_id_t>   ace_id;                                                 /* ACE ID */
    int                     ifmux_id;                                               /* Interface mux ID. */
    Bitmask                 bitmask[ACL_BITMASK_ARRAY_SIZE];                        /* ACE bitmask data */

    /* Reset the stored database */
    void reset_data()
    {
        ace_id.clear();
        ifmux_id = 0;
        memset(bitmask, 0, sizeof(bitmask));
    }

    /* Generate ifmux rule */
    void gen_rule(const Bitmask *new_bitmask, Rule &rule)
    {
        Bitmask empty_bitmask = {};

        for (int idx = 0; idx < ACL_BITMASK_ARRAY_SIZE; ++idx) {
            if (memcmp(&new_bitmask[idx], &empty_bitmask, sizeof(empty_bitmask))) {
                rule.emplace_back(element_acl_id(new_bitmask[idx]));
            }
        }
    }

    /* Lookup if ACE ID existing or getnext existing ACE ID */
    BOOL ace_lookup(mesa_ace_id_t *search_id, BOOL getnext)
    {
        if (getnext) { // GETNEXT
            sort(ace_id.begin(), ace_id.end());
            Vector<mesa_ace_id_t>::iterator iter = find_if(ace_id.begin(),
                                                           ace_id.end(),
            [&](mesa_ace_id_t const & id) {
                return id > *search_id;
            });
            if (iter) {
                *search_id = *iter;
            }
            return iter ? TRUE : FALSE;
        } else { // GET
            Vector<mesa_ace_id_t>::iterator iter = find(ace_id.begin(), ace_id.end(), *search_id);
            return iter ? TRUE : FALSE;
        }
    }
};

static critd_t acl_ifmux_crit;

#define ACL_IFMUX_CRIT_ENTER() critd_enter(&acl_ifmux_crit, __FILE__, __LINE__)
#define ACL_IFMUX_CRIT_EXIT()  critd_exit( &acl_ifmux_crit, __FILE__, __LINE__)

static ace_ifmux_t ACL_ifmux;
static mesa_rc acl_ifmux_add(mesa_ace_id_t ace_id);
static mesa_rc acl_ifmux_del(mesa_ace_id_t ace_id);
#endif /* defined(VTSS_SW_OPTION_IP) */

#define ACL_USER_INTERMAL_RESERVED  ACL_USER_CNT // Use the last one user ID for the reservation
#define ACL_DEF_PORT_ACE_ID_START   ACL_COMBINED_ID_SET(ACL_USER_INTERMAL_RESERVED, 0)

#ifdef VTSS_SW_OPTION_IP
/* The API is needed in Lu26 chipset only.
 * Whenever an ACE change (add/delete) has been done through the VTSS
 * ACL API (for an ACL rule or a default port rule), IFH.ACL_IDX may
 * be changed. The ACL module must go through all rules again.
*/
static mesa_rc acl_ifmux_update(void);
#endif

uint32_t acl_cap_ace_cnt_get(void)
{
    static uint32_t val;

    // Cache value of VTSS_APPL_CAP_ACL_ACE_CNT, because the call to fast_cap()
    // is pretty expensive in terms of time.
    if (!val) {
        val = fast_cap(VTSS_APPL_CAP_ACL_ACE_CNT);
    }

    return val;
}

/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/

/* Determine if ACL port configuration has changed */
int acl_mgmt_port_conf_changed(acl_port_conf_t *old, acl_port_conf_t *new_conf)
{
    return (memcmp(new_conf, old, sizeof(*new_conf)));
}

/* Set ACL port defaults */
void acl_mgmt_port_conf_get_default(acl_port_conf_t *conf)
{
    vtss_clear(*conf);
    conf->policy_no = 0;
    conf->action.policer = ACL_MGMT_RATE_LIMITER_NONE;

    conf->action.port_action = MESA_ACL_PORT_ACTION_NONE;
    memset(conf->action.port_list, 0x0, sizeof(conf->action.port_list));
    conf->action.mirror = FALSE;
    conf->action.ptp_action = MESA_ACL_PTP_ACTION_NONE;
    conf->action.logging = FALSE;
    conf->action.shutdown = FALSE;
    conf->action.force_cpu = FALSE;
    conf->action.cpu_once = FALSE;
    conf->action.cpu_queue = PACKET_XTR_QU_ACL_COPY; /* Default ACL queue */
}

/* Set ACL action */
static void acl_action_set(mesa_ace_id_t id, mesa_acl_action_t *action, acl_action_t *new_conf)
{
    port_iter_t pit;
#if PACKET_XTR_QU_ACL_REDIR != PACKET_XTR_QU_ACL_COPY
    BOOL        cpu_copy = FALSE;
#endif /* PACKET_XTR_QU_ACL_REDIR != PACKET_XTR_QU_ACL_COPY */

    vtss_clear(*action);

    /* If the ACE covers multiple ports, which must be shut down, we must send it all to CPU */
    action->cpu = (new_conf->force_cpu || new_conf->logging || new_conf->shutdown ? 1 : 0);
    action->cpu_once = new_conf->cpu_once;
    action->cpu_queue = new_conf->cpu_queue;

    action->police = (new_conf->policer == ACL_MGMT_RATE_LIMITER_NONE ? 0 : 1);
    action->policer_no = new_conf->policer;

    action->port_action = new_conf->port_action;
    action->learn = 1;
    port_iter_init_local(&pit);
    while (port_iter_getnext(&pit)) {
        action->port_list[pit.iport] = new_conf->port_list[pit.iport];
#if PACKET_XTR_QU_ACL_REDIR != PACKET_XTR_QU_ACL_COPY
        if (action->port_list[pit.iport]) {
            cpu_copy = TRUE;
        }
#endif /* PACKET_XTR_QU_ACL_REDIR != PACKET_XTR_QU_ACL_COPY */
    }
    if (new_conf->inject_manual) {
        action->ifh_flag = !new_conf->inject_into_ip_stack;
    } else {
        if (action->port_action != MESA_ACL_PORT_ACTION_NONE) {
            /* Set IFH flag for discard action */
            action->ifh_flag = TRUE;
            if (!new_conf->logging && !new_conf->shutdown) {
                port_iter_init_local(&pit);
                while (port_iter_getnext(&pit)) {
                    if (action->port_list[pit.iport]) {
                        action->ifh_flag = FALSE;
                        break;
                    }
                }
            }
        }
    }
    action->mirror = new_conf->mirror;
    action->ptp_action = new_conf->ptp_action;
    action->ptp = new_conf->ptp;
    action->addr = new_conf->addr;

#if PACKET_XTR_QU_ACL_REDIR != PACKET_XTR_QU_ACL_COPY
    if (action->cpu || action->cpu_once) {
        // When CPU-copying or redirecting on stackable and/or 48-ported JR1,
        // we must ensure that we don't get into a CPU queue that gets forwarded
        // from a secondary switch or device to the current primary switch or
        // device). We redirect only PACKET_XTR_QU_L3_OTHER and
        // PACKET_XTR_QU_MGMT_MAC to the current primary switch.
        if (action->cpu_queue == PACKET_XTR_QU_L3_OTHER || action->cpu_queue == PACKET_XTR_QU_MGMT_MAC) {
            mesa_packet_rx_queue_t new_cpu_queue = cpu_copy ? PACKET_XTR_QU_ACL_COPY : PACKET_XTR_QU_ACL_REDIR;

            T_W("Cannot use %u as ACL %s port, since it gets redirected to current primary switch. Using %u instead", action->cpu_queue, cpu_copy ? "copy" : "redirect", new_cpu_queue);
            action->cpu_queue = new_cpu_queue;
        }
    }
#endif /* PACKET_XTR_QU_ACL_REDIR != PACKET_XTR_QU_ACL_COPY */

    action->lm_cnt_disable = new_conf->lm_cnt_disable;
    action->mac_swap = new_conf->mac_swap;
}

/* Setup ACL port configuration via switch API */
static mesa_rc acl_port_conf_set(mesa_port_no_t port_no, acl_port_conf_t *conf)
{
    mesa_acl_port_conf_t port_conf = {};

    port_conf.policy_no = conf->policy_no;
    acl_action_set(ACL_MGMT_ACE_ID_NONE, &port_conf.action, &conf->action);
    return mesa_acl_port_conf_set(NULL, port_no, &port_conf);
}

/* Determine if ACL policer configuration has changed */
int acl_mgmt_policer_conf_changed(const vtss_appl_acl_config_rate_limiter_t *const old, const vtss_appl_acl_config_rate_limiter_t *const new_conf)
{
    if (new_conf->bit_rate_enable == old->bit_rate_enable) {
        if (new_conf->bit_rate_enable) {
            return (new_conf->bit_rate != old->bit_rate);
        } else {
            return (new_conf->packet_rate != old->packet_rate);
        }
    }
    return 1;
}

/* Get ACL policer defaults */
void acl_mgmt_policer_conf_get_default(vtss_appl_acl_config_rate_limiter_t *conf)
{
    memset(conf, 0, sizeof(*conf));
    conf->bit_rate_enable = FALSE;
    conf->bit_rate = 0;
    conf->packet_rate = (ACL_MGMT_PACKET_RATE_GRANULARITY_SMALL_RANGE ? 1 :
                         ACL_MGMT_PACKET_RATE_GRANULARITY);
}

/* Setup policer configuration via switch API */
static mesa_rc acl_policer_conf_apply(mesa_acl_policer_no_t policer_no,
                                      vtss_appl_acl_config_rate_limiter_t *conf)
{
    mesa_acl_policer_conf_t policer_conf;

    policer_conf.bit_rate_enable = conf->bit_rate_enable;
    policer_conf.bit_rate = conf->bit_rate;
    policer_conf.rate = conf->packet_rate;

    return mesa_acl_policer_conf_set(NULL, policer_no, &policer_conf);
}

/* Convert flag to mesa_ace_bit_t type */
static mesa_ace_bit_t acl_bit_get(acl_flag_t flag, acl_entry_conf_t *conf)
{
    return (VTSS_BF_GET(conf->flags.mask, flag) ?
            (VTSS_BF_GET(conf->flags.value, flag) ?
             MESA_ACE_BIT_1 : MESA_ACE_BIT_0) : MESA_ACE_BIT_ANY);
}

/* Add ACE via switch API */
static mesa_rc acl_ace_add(mesa_ace_id_t id, acl_entry_conf_t *conf)
{
    mesa_rc       rc;
    mesa_ace_t    ace;
    port_iter_t   pit;
    mesa_ace_id_t id_next = id;

    T_D("enter, id: %d, type: %d", id, conf->type);
    if (mesa_ace_init(NULL, conf->type, &ace) != VTSS_RC_OK) {
        T_W("Calling mesa_ace_init() failed\n");
    }
    ace.id = conf->id;
    ace.lookup = conf->lookup;
    ace.isdx_enable = conf->isdx_enable;
    ace.isdx_disable = conf->isdx_disable;
    ace.policy = conf->policy;
    acl_action_set(conf->id, &ace.action, &conf->action);
    ace.dmac_mc = acl_bit_get(ACE_FLAG_DMAC_MC, conf);
    ace.dmac_bc = acl_bit_get(ACE_FLAG_DMAC_BC, conf);
    ace.vlan.vid = conf->vid;
    ace.vlan.usr_prio = conf->usr_prio;
    ace.vlan.cfi = acl_bit_get(ACE_FLAG_VLAN_CFI, conf);
    ace.vlan.tagged = conf->tagged;
    port_iter_init_local(&pit);
    while (port_iter_getnext(&pit)) {
        ace.port_list[pit.iport] = conf->port_list[pit.iport];
    }

    switch (conf->type) {
    case MESA_ACE_TYPE_ETYPE:
        ace.frame.etype.dmac = conf->frame.etype.dmac;
        ace.frame.etype.smac = conf->frame.etype.smac;
        ace.frame.etype.etype = conf->frame.etype.etype;
        ace.frame.etype.data = conf->frame.etype.data;
        ace.frame.etype.ptp = conf->frame.etype.ptp;
        break;
    case MESA_ACE_TYPE_LLC:
        ace.frame.llc.dmac = conf->frame.llc.dmac;
        ace.frame.llc.smac = conf->frame.llc.smac;
        ace.frame.llc.llc = conf->frame.llc.llc;
        break;
    case MESA_ACE_TYPE_SNAP:
        ace.frame.snap.dmac = conf->frame.snap.dmac;
        ace.frame.snap.smac = conf->frame.snap.smac;
        ace.frame.snap.snap = conf->frame.snap.snap;
        break;
    case MESA_ACE_TYPE_ARP:
        ace.frame.arp.smac = conf->frame.arp.smac;
        ace.frame.arp.arp = acl_bit_get(ACE_FLAG_ARP_ARP, conf);
        ace.frame.arp.req = acl_bit_get(ACE_FLAG_ARP_REQ, conf);
        ace.frame.arp.unknown = acl_bit_get(ACE_FLAG_ARP_UNKNOWN, conf);
        ace.frame.arp.smac_match = acl_bit_get(ACE_FLAG_ARP_SMAC, conf);
        ace.frame.arp.dmac_match = acl_bit_get(ACE_FLAG_ARP_DMAC, conf);
        ace.frame.arp.length = acl_bit_get(ACE_FLAG_ARP_LEN, conf);
        ace.frame.arp.ip = acl_bit_get(ACE_FLAG_ARP_IP, conf);
        ace.frame.arp.ethernet = acl_bit_get(ACE_FLAG_ARP_ETHER, conf);
        ace.frame.arp.sip = conf->frame.arp.sip;
        ace.frame.arp.dip = conf->frame.arp.dip;
        break;
    case MESA_ACE_TYPE_IPV4:
        ace.frame.ipv4.ttl = acl_bit_get(ACE_FLAG_IP_TTL, conf);
        ace.frame.ipv4.fragment = acl_bit_get(ACE_FLAG_IP_FRAGMENT, conf);
        ace.frame.ipv4.options = acl_bit_get(ACE_FLAG_IP_OPTIONS, conf);
        ace.frame.ipv4.ds = conf->frame.ipv4.ds;
        ace.frame.ipv4.proto = conf->frame.ipv4.proto;
        ace.frame.ipv4.sip = conf->frame.ipv4.sip;
        ace.frame.ipv4.dip = conf->frame.ipv4.dip;
        if (ace.frame.ipv4.options != MESA_ACE_BIT_1 &&
            ace.frame.ipv4.fragment != MESA_ACE_BIT_1) {
            /* IPv4 payload and UDP/TCP header ignored if IP option/fragment */
            ace.frame.ipv4.data = conf->frame.ipv4.data;
            ace.frame.ipv4.sport = conf->frame.ipv4.sport;
            ace.frame.ipv4.dport = conf->frame.ipv4.dport;
            ace.frame.ipv4.tcp_fin = acl_bit_get(ACE_FLAG_TCP_FIN, conf);
            ace.frame.ipv4.tcp_syn = acl_bit_get(ACE_FLAG_TCP_SYN, conf);
            ace.frame.ipv4.tcp_rst = acl_bit_get(ACE_FLAG_TCP_RST, conf);
            ace.frame.ipv4.tcp_psh = acl_bit_get(ACE_FLAG_TCP_PSH, conf);
            ace.frame.ipv4.tcp_ack = acl_bit_get(ACE_FLAG_TCP_ACK, conf);
            ace.frame.ipv4.tcp_urg = acl_bit_get(ACE_FLAG_TCP_URG, conf);
        }
        ace.frame.ipv4.sip_smac = conf->frame.ipv4.sip_smac;
        ace.frame.ipv4.ptp = conf->frame.ipv4.ptp;
        break;
    case MESA_ACE_TYPE_IPV6:
        ace.frame.ipv6.proto = conf->frame.ipv6.proto;
        ace.frame.ipv6.sip = conf->frame.ipv6.sip;
        ace.frame.ipv6.ttl = conf->frame.ipv6.ttl;
        ace.frame.ipv6.ds = conf->frame.ipv6.ds;
        ace.frame.ipv6.data = conf->frame.ipv6.data;
        ace.frame.ipv6.sport = conf->frame.ipv6.sport;
        ace.frame.ipv6.dport = conf->frame.ipv6.dport;
        ace.frame.ipv6.tcp_fin = conf->frame.ipv6.tcp_fin;
        ace.frame.ipv6.tcp_syn = conf->frame.ipv6.tcp_syn;
        ace.frame.ipv6.tcp_rst = conf->frame.ipv6.tcp_rst;
        ace.frame.ipv6.tcp_psh = conf->frame.ipv6.tcp_psh;
        ace.frame.ipv6.tcp_ack = conf->frame.ipv6.tcp_ack;
        ace.frame.ipv6.tcp_urg = conf->frame.ipv6.tcp_urg;
        ace.frame.ipv6.ptp = conf->frame.ipv6.ptp;
        break;
    case MESA_ACE_TYPE_ANY:
        /* JR2: Filtering on DMAC/SMAC supported for ETYPE/LLC/SNAP */
        ace.frame.etype.dmac = conf->frame.etype.dmac;
        ace.frame.etype.smac = conf->frame.etype.smac;
        break;
    default:
        break;
    }

    if (acl.def_port_ace_supported && id == ACL_MGMT_ACE_ID_NONE && ace.id < ACL_DEF_PORT_ACE_ID_START) {
        id_next =  ACL_DEF_PORT_ACE_ID_START;
    }

    rc = mesa_ace_add(NULL, id_next, &ace);

#ifdef VTSS_SW_OPTION_IP
    if (acl.ifmux_supported && rc == VTSS_RC_OK) {
        BOOL is_permit = (conf->action.port_action == MESA_ACL_PORT_ACTION_NONE);
        if (!is_permit) {
            mesa_port_no_t port_no;
            u32            port_cnt = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);
            for (port_no = 0; port_no < port_cnt; port_no++) {
                if (conf->action.port_list[port_no]) {
                    is_permit = TRUE;   // Only process deny all action.
                    break;
                }
            }
        }

        int ifmux_id = 0;
        BOOL found_ifmux_ace = (acl_mgmt_ifmux_get(&ace.id, &ifmux_id, FALSE) == VTSS_RC_OK) ? TRUE : FALSE;
        if (!is_permit && (conf->action.logging || conf->action.shutdown)) {
            // Add new ifmux rule
            rc = acl_ifmux_add(ace.id);
        } else if (found_ifmux_ace) {
            // Delete ifmux rule
            rc = acl_ifmux_del(ace.id);
        }

        if (rc == VTSS_RC_OK) {
            (void)acl_ifmux_update();
        }
    }
#endif

    T_D("exit, id: %d", id);

    return rc;
}

/* Delete ACE via switch API */
static mesa_rc acl_ace_del(mesa_ace_id_t id)
{
    mesa_rc rc;

    T_D("enter, id: %d", id);

#ifdef VTSS_SW_OPTION_IP
    if (acl.ifmux_supported) {
        rc = acl_ifmux_del(id);
        if (rc == VTSS_RC_OK) {
            (void)acl_ifmux_update();
        }
    }
#endif

    rc = mesa_ace_del(NULL, id);
    T_D("exit, id: %d", id);

    return rc;
}

/* Change ACE's conflict status */
static void acl_ace_conflict_set(mesa_ace_id_t ace_id, BOOL conflict)
{
    acl_ace_t     *ace;
    acl_list_t    *list;

    T_D("enter, ACE id: %d", ace_id);
    ACL_LOCK_ASSERT_LOCKED("acl_ace_conflict_set");
    list = &acl.switch_acl;
    for (ace = list->used; ace != NULL; ace = ace->next) {
        if (ace->conf.id == ace_id) { /* Found existing entry */
            ace->conf.conflict = conflict;
            break;
        }
    }
}

/*lint -e{593} */
/* There is a lint error message: Custodial pointer 'new' possibly not freed or returned.
   We skip the lint error cause we freed it in acl_list_ace_del() */
/* Add ACE to list */
static mesa_rc acl_list_ace_add(mesa_ace_id_t *next_id, acl_entry_conf_t *conf)
{
    mesa_rc       rc = VTSS_RC_OK;
    acl_ace_t     *ace, *prev, *ins = NULL, *ins_prev = NULL, *new_ace = NULL, *new_prev = NULL;
    acl_list_t    *list;
    uchar         id_used[VTSS_BF_SIZE(ACL_MGMT_ACE_ID_END)];
    mesa_ace_id_t i;
    int           isid_count[VTSS_ISID_END + 1], found_insertion_place = 0;
    BOOL          new_malloc = FALSE;

    T_D("enter, id: %d, conf->id: %d", *next_id, conf->id);
    ACL_LOCK_ASSERT_LOCKED("acl_list_ace_add");

    if (*next_id == conf->id && ACL_ACE_ID_GET(*next_id) != 0) {
        T_W("illegal user_id: %d, next_id: %d", conf->id, *next_id);
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    memset(id_used, 0, sizeof(id_used));
    memset(isid_count, 0, sizeof(isid_count));
    isid_count[conf->isid]++;

    list = &acl.switch_acl;

    /* Lookup the right place in the ACEs list. */
    for (ace = list->used, prev = NULL; ace != NULL; prev = ace, ace = ace->next) {
        if (ace->conf.id == conf->id) {
            /* Found existing entry */
            new_ace = ace;
            new_prev = prev;
        } else {
            isid_count[ace->conf.isid]++;
        }

        if (ace->conf.id == *next_id) {
            /* Found insertion place */
            ins = ace;
            ins_prev = prev;
        } else if (ACL_ACE_ID_GET(*next_id) == ACL_MGMT_ACE_ID_NONE) {
            if (found_insertion_place == 0 && ACL_USER_ID_GET(conf->id) > ACL_USER_ID_GET(ace->conf.id)) {
                /* Found insertion place by ordered priority */
                ins = ace;
                ins_prev = prev;
                found_insertion_place = 1;
            }
        }

        /* Mark ID as used */
        if (ACL_USER_ID_GET(conf->id) == ACL_USER_ID_GET(ace->conf.id)) {
            VTSS_BF_SET(id_used, ACL_ACE_ID_GET(ace->conf.id) - ACL_MGMT_ACE_ID_START, 1);
        }
    }

    if (ACL_ACE_ID_GET(*next_id) == ACL_MGMT_ACE_ID_NONE && found_insertion_place == 0) {
        ins_prev = prev;
    }

    /* Check that the insert ID was found */
    if (ACL_ACE_ID_GET(*next_id) != ACL_MGMT_ACE_ID_NONE && ins == NULL) {
        T_W("user_id: %d, next_id: %d not found", conf->id, *next_id);
        rc = VTSS_APPL_ACL_ERROR_ACE_NOT_FOUND;
    }

    /* Check that a new entry can be allocated */
    if (rc == VTSS_RC_OK && new_ace == NULL && list->free == NULL) {
        /* The maximum number of entries supported by the hardware is exceeded.
           We alloc a new memory for saving this entry and turn on "conflict" flag */
        if ((new_ace = (acl_ace_t *)VTSS_MALLOC(sizeof(acl_ace_t))) == NULL) {
            T_D("exit");
            return VTSS_APPL_ACL_ERROR_MEM_ALLOC_FAIL;
        }
        new_ace->conf.new_allocated = TRUE;
        new_ace->conf.conflict = TRUE;
        new_ace->next = NULL;
        new_malloc = TRUE;
    }

    if (rc == VTSS_RC_OK) {
        T_D("using from %s list", new_ace == NULL ? "free" : "used");
        if (new_ace == NULL) {
            /* Use first entry in free list */
            new_ace = list->free;
            if (new_ace) {
                new_ace->conf.new_allocated = FALSE;
                new_ace->conf.conflict = FALSE;
                list->free = new_ace->next;
            }
        } else if (new_malloc == FALSE) {
            /* Take existing entry out of used list */
            if (ins_prev == new_ace) {
                ins_prev = new_prev;
            }
            if (new_prev == NULL) {
                list->used = new_ace->next;
            } else {
                new_prev->next = new_ace->next;
            }
        }

        /* Insert new entry in used list */
        if (new_ace) {
            if (ACL_ACE_ID_GET(conf->id) == ACL_MGMT_ACE_ID_NONE) {
                /* Use next available ID */
                for (i = ACL_MGMT_ACE_ID_START; i <= ACL_MGMT_ACE_ID_END; i++) {
                    if (!VTSS_BF_GET(id_used, i - ACL_MGMT_ACE_ID_START)) {
                        conf->id = ACL_COMBINED_ID_SET(ACL_USER_ID_GET(conf->id), i);
                        break;
                    }
                }
                if (i > ACL_MGMT_ACE_ID_END) {
                    T_D("ACE Auto-assigned fail");
                    if (new_malloc == TRUE) {
                        VTSS_FREE(new_ace);
                    }
                    return VTSS_APPL_ACL_ERROR_ACE_AUTO_ASSIGNED_FAIL;
                }
            }

            if (ins_prev == NULL) {
                T_D("inserting first");
                new_ace->next = list->used;
                list->used = new_ace;
            } else {
                T_D("inserting after ID %d", ins_prev->conf.id);
                new_ace->next = ins_prev->next;
                ins_prev->next = new_ace;
            }

            conf->new_allocated = new_ace->conf.new_allocated;
            conf->conflict = new_ace->conf.conflict;
            new_ace->conf = *conf;

            /* Update the next_id, it must existing on chip layer when we call acl_ace_add() */
            *next_id = ACL_MGMT_ACE_ID_NONE;
            while (new_ace->next) {
                if (!new_ace->next->conf.conflict) {
                    *next_id = new_ace->next->conf.id;
                    break;
                }
                new_ace = new_ace->next;
            }
            T_D("next_id: %d, conf->id: %d", *next_id, conf->id);
        } else {
            // It should never happpen new = NULL
            T_E("Found new = NULL in acl_list_ace_add()\n");
            rc = VTSS_APPL_ACL_ERROR_GEN;
        }
    }

    if (rc != VTSS_RC_OK && new_malloc == TRUE) {
        VTSS_FREE(new_ace);
    }

    return rc;
}

static mesa_rc acl_list_ace_del(mesa_ace_id_t id, BOOL *conflict)
{
    acl_ace_t  *ace, *prev;
    acl_list_t *list;
    BOOL        found = FALSE;

    T_D("enter, id: %d", id);
    ACL_LOCK_ASSERT_LOCKED("acl_list_ace_del");
    list = &acl.switch_acl;
    for (ace = list->used, prev = NULL; ace != NULL; prev = ace, ace = ace->next) {
        T_D("Searching  ace->conf.id == id: %d %d", ace->conf.id,  id);
        if (ace->conf.id == id) {
            if (prev == NULL) {
                list->used = ace->next;
            } else {
                prev->next = ace->next;
            }
            *conflict = ace->conf.conflict;
            if (ace->conf.new_allocated) {
                /* This entry is saving in new memory */
                T_D("FREE ace");
                VTSS_FREE(ace);
            } else {
                /* Move entry from used list to free list */
                T_D("MOVE ace");
                ace->next = list->free;
                list->free = ace;
            }
            found = TRUE;
            break;
        }
    }

    if (found) {
        T_D("exit, id: %d found", id);
    } else {
        T_D("exit, id: %d not found", id);
    }
    return (found ? VTSS_RC_OK : (mesa_rc)VTSS_APPL_ACL_ERROR_ACE_NOT_FOUND);/* FIXME: Using two different annonymous enums, where returned 'VTSS_APPL_ACL_ERROR_ACE_NOT_FOUND' is out of 'mesa_rc' */
}

/* Get ACE or next ACE (use ACL_MGMT_ACE_ID_NONE to get first) */
mesa_rc acl_list_ace_get(mesa_ace_id_t id, acl_entry_conf_t *conf,
                         mesa_ace_counter_t *counter, BOOL next)
{
    mesa_rc    rc = VTSS_RC_OK;
    acl_ace_t  *ace;
    acl_list_t *list;
    BOOL       use_next = 0;

    T_D("enter, id: %d, next: %d", id, next);

    ACL_LOCK_ASSERT_LOCKED("acl_list_ace_get");
    list = &acl.switch_acl;
    for (ace = list->used; ace != NULL; ace = ace->next) {

        if (ACL_USER_ID_GET(ace->conf.id) != ACL_USER_ID_GET(id)) {
            /* Skip ACEs for other users */
            continue;
        }

        if (ACL_ACE_ID_GET(id) == ACL_MGMT_ACE_ID_NONE) {
            /* Found first ACE */
            T_D("Found first id: %d, conf: %d", id, ace->conf.id);
            break;
        }

        if (use_next) {
            /* Found next ACE */
            T_D("Found use_next:  first id: %d, conf: %d", id, ace->conf.id);
            break;
        }

        if (ace->conf.id == id) {
            T_D("Found egual id: %d, conf: %d next %u", id, ace->conf.id, next);
            /* Found ACE */
            if (next) {
                use_next = 1;
            } else {
                break;
            }
        }
    }
    if (ace != NULL) {
        /* Found it */
        *conf = ace->conf;
        T_D("Found id: %d  conf: %d", id, ace->conf.id);
    } else {
        T_D("ACL entry not found id:%d", id);
        return VTSS_APPL_ACL_ERROR_ACE_NOT_FOUND;
    }

    /* Get counter */
    if (counter) {
        *counter = 0;
    }
    if (conf->conflict == FALSE && counter != NULL) {
        rc = mesa_ace_counter_get(NULL, conf->id, counter);
    }

    T_D("Return rc:%u id: %d conf: %d", rc,  id, ace->conf.id);
    return rc;
}

/* Get ACE or next ACE by precedence
   Only provide static ACEs when the input parameter isid = VTSS_ISID_GLOBAL
 */
static mesa_rc acl_list_ace_get_nth(
    vtss_isid_t         isid,
    u32                 precedence,
    BOOL                b_next,
    acl_entry_conf_t    *conf,
    mesa_ace_counter_t  *counter
)
{
    mesa_rc    rc = VTSS_RC_OK;
    acl_ace_t  *ace;
    acl_list_t *list;
    BOOL       use_next = 0;
    u32        ace_cnt = 0;

    T_D("enter, isid: %d, precedence: %u, b_next: %s", isid, precedence, b_next ? "TRUE" : "FALSE");

    ACL_LOCK_SCOPE();
    list = &acl.switch_acl;
    for (ace = list->used; ace != NULL; ace = ace->next) {

        if (isid == VTSS_ISID_GLOBAL &&
            ACL_USER_ID_GET(ace->conf.id) != ACL_USER_STATIC) {
            continue;
        }

        if (precedence == 0 && b_next) {
            /* Found first ACE */
            break;
        }

        if (use_next) {
            /* Found next ACE */
            break;
        }

        if (++ace_cnt == precedence) {
            /* Found ACE */
            if (b_next) {
                use_next = 1;
            } else {
                break;
            }
        }
    }
    if (ace != NULL) {
        /* Found it */
        *conf = ace->conf;
    } else {
        return VTSS_APPL_ACL_ERROR_ACE_NOT_FOUND;
    }
    /* Get counter */
    if (counter) {
        *counter = 0;
    }
    if (conf->conflict == FALSE && counter != NULL) {
        rc = mesa_ace_counter_get(NULL, conf->id, counter);
    }

    T_D("exit rc:%u", rc);
    return rc;
}

static void acl_counters_clear(void)
{
    acl_ace_t      *ace;
    mesa_port_no_t port_no;
    u32            port_count = port_count_max();

    T_D("enter");

    /* Clear port counters */
    for (port_no = 0; port_no < port_count; port_no++) {
        if (acl.def_port_ace_supported) {
            (void) mesa_ace_counter_clear(NULL, ACL_DEF_PORT_ACE_ID_START + port_no);
        } else {
            (void) mesa_acl_port_counter_clear(NULL, port_no);
        }
    }

    /* Clear ACE counters */
    ACL_CRIT_ENTER();
    for (ace = acl.switch_acl.used; ace != NULL; ace = ace->next) {
        if (ace->conf.conflict == FALSE) {
            (void) mesa_ace_counter_clear(NULL, ace->conf.id);
        }
    }
    ACL_CRIT_EXIT();

    T_D("exit");
}

/****************************************************************************/
/*  Stack/switch functions                                                  */
/****************************************************************************/

/* Set default port ACE on local switch */
static mesa_rc acl_default_port_ace_set(mesa_port_no_t port_no, acl_action_t *action)
{
    mesa_rc          rc;
    acl_entry_conf_t ace;
    mesa_ace_id_t    id_next;
    u32              port_cnt = port_count_max();

    if ((rc = acl_mgmt_ace_init(MESA_ACE_TYPE_ANY, &ace)) != VTSS_RC_OK) {
        return rc;
    }

    ace.id = ACL_DEF_PORT_ACE_ID_START + port_no;
    ace.action = *action;
    memset(ace.port_list, 0, sizeof(ace.port_list));
    ace.port_list[port_no] = TRUE;

    if (acl.def_port_ace_init[port_no] && port_no != (port_cnt - 1)) {
        id_next = (ACL_DEF_PORT_ACE_ID_START + port_no + 1);
    } else {
        id_next = ACL_MGMT_ACE_ID_NONE;
    }
    if ((rc = acl_ace_add(id_next, &ace)) == VTSS_RC_OK) {
        acl.def_port_ace_init[port_no] = TRUE;
    }

    return rc;
}

/* JSON notification */
static void acl_msg_tx_event(BOOL is_crossed)
{
    vtss_isid_t                      isid = VTSS_ISID_START;
    vtss_appl_acl_status_ace_event_t status;

    /* Send notification */
    status.crossed_threshold = is_crossed;
    acl_status_ace_event_update.set(topo_isid2usid(isid), &status);
    T_D("Send notification: isid=%d, Reached=%s", isid, status.crossed_threshold ? "T" : "F");
}

/* Set port configuration */
static mesa_rc acl_port_conf_set(void)
{
    u32             port_count = port_count_max();
    mesa_port_no_t  port_no;
    acl_port_conf_t *conf;

    T_D("enter");
    ACL_LOCK_ASSERT_LOCKED("acl_port_conf_set");
    for (port_no = 0; port_no < port_count; port_no++) {
        conf = &acl.port_conf[port_no];
        if (acl_port_conf_set(port_no, conf) != VTSS_RC_OK) {
            T_W("Calling acl_port_conf_set() failed\n");
        } else if (acl.def_port_ace_supported) {
            (void)acl_default_port_ace_set(port_no, &conf->action);
        }
    }
    acl_packet_register();
    T_D("exit");

    return VTSS_RC_OK;
}

/* Set ACL policer configuration */
static mesa_rc acl_policer_conf_set(void)
{
    u32                   pol_count = fast_cap(MESA_CAP_ACL_POLICER_CNT);
    mesa_acl_policer_no_t policer_no;

    T_D("enter");
    ACL_LOCK_ASSERT_LOCKED("acl_policer_conf_set");
    for (policer_no = 0; policer_no < pol_count; policer_no++) {
        if (acl_policer_conf_apply(policer_no, &acl.policer_conf[policer_no]) != VTSS_RC_OK) {
            T_W("acl_policer_conf_apply() failed\n");
        }
    }
    T_D("exit");
    return VTSS_RC_OK;
}

static mesa_rc acl_ace_conf_set()
{
    mesa_ace_id_t    id;
    acl_entry_conf_t conf;
    BOOL             conflict;

    T_D("enter");

    ACL_LOCK_ASSERT_LOCKED("acl_ace_conf_set");

    id = ACL_MGMT_ACE_ID_NONE;
    while (acl_list_ace_get(ACL_COMBINED_ID_SET(ACL_USER_STATIC, id), &conf, NULL, TRUE) == VTSS_RC_OK) {
        if (acl_list_ace_del(conf.id, &conflict) == VTSS_RC_OK && conflict == FALSE) {
            /* Remove this entry from ASIC layer */
            if (acl_ace_del(conf.id) != VTSS_RC_OK) {
                T_W("Calling acl_ace_del() failed\n");
            }
        }
    }

    acl_packet_register();

    T_D("exit");
    return VTSS_RC_OK;
}

/* Clear all ACL counters in stack */
static mesa_rc acl_stack_clear(void)
{
    acl_counters_clear();
    return VTSS_RC_OK;
}

#ifdef VTSS_SW_OPTION_PACKET
/* Shutdown port */
static void acl_stack_port_shutdown(mesa_port_no_t port_no)
{
    port_vol_conf_t conf;
    port_user_t     user = PORT_USER_ACL;

    T_D("shutdown, port_no: %u", port_no);
    if (port_no < port_count_max() && port_vol_conf_get(user, port_no, &conf) == VTSS_RC_OK) {
        conf.disable = 1;
        if (port_vol_conf_set(user, port_no, &conf) != VTSS_RC_OK) {
            T_W("port_vol_conf_set() failed");
        }
    }
}

static void acl_stack_port_restore()
{
    const port_user_t user = PORT_USER_ACL;
    const u32         port_count = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    for (mesa_port_no_t port_no = 0; port_no < port_count; ++port_no) {
        port_vol_conf_t conf = {};
        (void)port_vol_conf_get(user, port_no, &conf);
        if (conf.disable == 1) {
            conf.disable = 0;
            (void)port_vol_conf_set(user, port_no, &conf);
        }
    }
}

/****************************************************************************/
/*  Packet logging functions                                                */
/****************************************************************************/
static ushort acl_getb16(uchar *p)
{
    return (p[0] << 8 | p[1]);
}

static u32 acl_getb32(uchar *p)
{
    return ((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
}

static BOOL acl_flag_mismatch(acl_entry_conf_t *conf, uchar *flags, acl_flag_t flag)
{
    return (VTSS_BF_GET(conf->flags.mask, flag) &&
            VTSS_BF_GET(conf->flags.value, flag) != VTSS_BF_GET(flags, flag));
}

static BOOL acl_packet_rx(void *contxt, const uchar *const frame, const mesa_packet_rx_info_t *const rx_info)
{
    size_t               len;
    uint                 i, n;
    mesa_port_no_t       port_no;
    mesa_ace_type_t      type;
    uchar                *dmac, *smac, *p;
    BOOL                 bc, match;
    ushort               etype, sport = 0, dport = 0;
    uchar                vh, ds = 0, proto = 0, tcp_flags;
    u32                  sip = 0, dip = 0;
    acl_ace_t            *ace;
    acl_entry_conf_t     *conf;
    uchar                flags[ACE_FLAG_SIZE];
    acl_port_conf_t      *port_conf;
    acl_action_t         *action;
    vtss_mtimer_t        *timer;
    BOOL                 is_permit, found_matched = FALSE;
    mesa_vid_t           vid;
    struct ip6_hdr       *ip6_hdr = NULL;
    mesa_ipv6_t          sip_v6, dip_v6;

    memset(&sip_v6, 0, sizeof(sip_v6));
    memset(&dip_v6, 0, sizeof(dip_v6));

    len = rx_info->length;
    port_no = rx_info->port_no;
    vid = rx_info->tag.vid;
    T_N("Frame of length %zu received on port %u, vid: %u", len, port_no, vid);
    T_N_HEX(frame, len);

    /* Extract MAC header fields */
    p = (uchar *)frame;
    dmac = (p + 0);
    smac = (p + 6);
    etype = acl_getb16(p + 12);
    p += 14; /* Skip MAC header */

    /* Determine DMAC flags */
    memset(flags, 0, sizeof(flags));
    for (bc = 1, i = 0; i < 6; i++)
        if (dmac[i] != 0xff) {
            bc = 0;
        }
    VTSS_BF_SET(flags, ACE_FLAG_DMAC_BC, bc);
    VTSS_BF_SET(flags, ACE_FLAG_DMAC_MC, dmac[0] & 0x01);

    /* Extract frame type specific fields */
    switch (etype) {
    case 0x0800: /* IPv4 */
        vh = p[0];
        if ((vh & 0xf0) != 0x40) { /* Not IPv4 */
            type = MESA_ACE_TYPE_ETYPE;
            break;
        }
        type = MESA_ACE_TYPE_IPV4;
        ds = p[1];
        VTSS_BF_SET(flags, ACE_FLAG_IP_FRAGMENT, acl_getb16(p + 6) & 0x3fff);
        VTSS_BF_SET(flags, ACE_FLAG_IP_TTL, p[8]);
        VTSS_BF_SET(flags, ACE_FLAG_IP_OPTIONS, vh != 0x45);
        proto = p[9];
        sip = acl_getb32(p + 12);
        dip = acl_getb32(p + 16);
        p += ((vh & 0x0f) * 4); /* Skip IP header */
        sport = acl_getb16(p);
        dport = acl_getb16(p + 2);
        if (proto == 6) {
            tcp_flags = p[13];
            VTSS_BF_SET(flags, ACE_FLAG_TCP_FIN, tcp_flags & 0x01);
            VTSS_BF_SET(flags, ACE_FLAG_TCP_SYN, tcp_flags & 0x02);
            VTSS_BF_SET(flags, ACE_FLAG_TCP_RST, tcp_flags & 0x04);
            VTSS_BF_SET(flags, ACE_FLAG_TCP_PSH, tcp_flags & 0x08);
            VTSS_BF_SET(flags, ACE_FLAG_TCP_ACK, tcp_flags & 0x10);
            VTSS_BF_SET(flags, ACE_FLAG_TCP_URG, tcp_flags & 0x20);
        }
        break;
    case 0x0806: /* ARP/RARP */
    case 0x8035:
        type = MESA_ACE_TYPE_ARP;

        /* Opcode flags */
        switch (acl_getb16(p + 6)) {
        case 1:  /* ARP request */
            VTSS_BF_SET(flags, ACE_FLAG_ARP_ARP, 1);
            VTSS_BF_SET(flags, ACE_FLAG_ARP_REQ, 1);
            break;
        case 2:  /* ARP reply */
            VTSS_BF_SET(flags, ACE_FLAG_ARP_ARP, 1);
            break;
        case 3:  /* RARP request */
            VTSS_BF_SET(flags, ACE_FLAG_ARP_REQ, 1);
            break;
        case 4:  /* RARP reply */
            break;
        default: /* Unknown */
            VTSS_BF_SET(flags, ACE_FLAG_ARP_UNKNOWN, 1);
            break;
        }

        /* SMAC flag */
        for (i = 0, match = 1; i < 6; i++)
            if (smac[i] != p[8 + i]) {
                match = 0;
            }
        VTSS_BF_SET(flags, ACE_FLAG_ARP_SMAC, match);

        /* DMAC flag */
        for (i = 0, match = 1; i < 6; i++)
            if (dmac[i] != p[18 + i]) {
                match = 0;
            }
        VTSS_BF_SET(flags, ACE_FLAG_ARP_DMAC, match);

        /* Length, IP and Ethernet flags */
        VTSS_BF_SET(flags, ACE_FLAG_ARP_LEN, p[4] == 6 && p[5] == 4);
        VTSS_BF_SET(flags, ACE_FLAG_ARP_IP, acl_getb16(p + 2) == 0x0800);
        VTSS_BF_SET(flags, ACE_FLAG_ARP_ETHER, acl_getb16(p + 0) == 0x0001);

        /* SIP/DIP */
        sip = acl_getb32(p + 14);
        dip = acl_getb32(p + 24);
        break;
    case 0x86DD: /* IPv6 */
        vh = p[0];
        if ((vh & 0xf0) != 0x60) { /* Not IPv6 */
            type = MESA_ACE_TYPE_ETYPE;
            break;
        }
        type = MESA_ACE_TYPE_IPV6;
        ip6_hdr = (struct ip6_hdr *)p;
        memcpy(&sip_v6, ip6_hdr->ip6_src.s6_addr, sizeof(sip_v6));
        memcpy(&dip_v6, ip6_hdr->ip6_dst.s6_addr, sizeof(dip_v6));

        ds = p[1];
        VTSS_BF_SET(flags, ACE_FLAG_IP_TTL, ip6_hdr->ip6_ctlun.ip6_un1.ip6_un1_hlim);
        proto = ip6_hdr->ip6_ctlun.ip6_un1.ip6_un1_nxt;
        p += sizeof(struct ip6_hdr); /* Skip IPv6 header */
        sport = acl_getb16(p);
        dport = acl_getb16(p + 2);
        if (proto == 6) {
            tcp_flags = p[13];
            VTSS_BF_SET(flags, ACE_FLAG_TCP_FIN, tcp_flags & 0x01);
            VTSS_BF_SET(flags, ACE_FLAG_TCP_SYN, tcp_flags & 0x02);
            VTSS_BF_SET(flags, ACE_FLAG_TCP_RST, tcp_flags & 0x04);
            VTSS_BF_SET(flags, ACE_FLAG_TCP_PSH, tcp_flags & 0x08);
            VTSS_BF_SET(flags, ACE_FLAG_TCP_ACK, tcp_flags & 0x10);
            VTSS_BF_SET(flags, ACE_FLAG_TCP_URG, tcp_flags & 0x20);
        }
        break;
    default:
        if (etype >= 0x0600) {
            type = MESA_ACE_TYPE_ETYPE;
        } else if (p[0] == 0xaa && p[1] == 0xaa && p[2] == 0x03) {
            type = MESA_ACE_TYPE_SNAP;
        } else {
            type = MESA_ACE_TYPE_LLC;
        }
        break;
    }

    ACL_CRIT_ENTER();

    /* Look for matching ACE */
    port_conf = &acl.port_conf[port_no];
    action = &port_conf->action;
    for (ace = acl.switch_acl.used; ace != NULL && ace->conf.conflict == FALSE; ace = ace->next) {
        conf = &ace->conf;

        /* Rule type */
        if (conf->port_list[port_no] != TRUE ||
            ((conf->policy.value & conf->policy.mask) != (port_conf->policy_no & conf->policy.mask))) {
            continue;
        }

        /* DMAC flags */
        if (acl_flag_mismatch(conf, flags, ACE_FLAG_DMAC_BC) ||
            acl_flag_mismatch(conf, flags, ACE_FLAG_DMAC_MC)) {
            continue;
        }

        /* VLAN ID */
        if ((vid & conf->vid.mask) != (conf->vid.value & conf->vid.mask)) {
            continue;
        }

        /* Frame type */
        if (conf->type == MESA_ACE_TYPE_ANY) {
            type = MESA_ACE_TYPE_ANY;
        } else if (conf->type != type) {
            continue;
        }

        switch (type) {
        case MESA_ACE_TYPE_ANY:
            break;
        case MESA_ACE_TYPE_ETYPE:
            /* SMAC/DMAC */
            for (i = 0, match = 1; i < 6; i++)
                if ((dmac[i] & conf->frame.etype.dmac.mask[i]) !=
                    (conf->frame.etype.dmac.value[i] & conf->frame.etype.dmac.mask[i]) ||
                    (smac[i] & conf->frame.etype.smac.mask[i]) !=
                    (conf->frame.etype.smac.value[i] & conf->frame.etype.smac.mask[i])) {
                    match = 0;
                }
            if (!match) {
                continue;
            }

            /* Ethernet Type */
            if (((etype >> 8) & conf->frame.etype.etype.mask[0]) !=
                (conf->frame.etype.etype.value[0] & conf->frame.etype.etype.mask[0]) ||
                (etype & conf->frame.etype.etype.mask[1]) !=
                (conf->frame.etype.etype.value[1] & conf->frame.etype.etype.mask[1])) {
                continue;
            }

            /* Ethernet data */
            for (i = 0, match = 1; i < 2; i++)
                if ((p[i] & conf->frame.etype.data.mask[i]) !=
                    (conf->frame.etype.data.value[i] & conf->frame.etype.data.mask[i])) {
                    match = 0;
                }
            if (!match) {
                continue;
            }
            break;
        case MESA_ACE_TYPE_ARP:
            /* ARP flags */
            if (acl_flag_mismatch(conf, flags, ACE_FLAG_ARP_ARP) ||
                acl_flag_mismatch(conf, flags, ACE_FLAG_ARP_REQ) ||
                acl_flag_mismatch(conf, flags, ACE_FLAG_ARP_UNKNOWN) ||
                acl_flag_mismatch(conf, flags, ACE_FLAG_ARP_SMAC) ||
                acl_flag_mismatch(conf, flags, ACE_FLAG_ARP_DMAC) ||
                acl_flag_mismatch(conf, flags, ACE_FLAG_ARP_LEN) ||
                acl_flag_mismatch(conf, flags, ACE_FLAG_ARP_IP) ||
                acl_flag_mismatch(conf, flags, ACE_FLAG_ARP_ETHER)) {
                continue;
            }

            /* SMAC */
            for (i = 0, match = 1; i < 6; i++)
                if ((smac[i] & conf->frame.arp.smac.mask[i]) !=
                    (conf->frame.arp.smac.value[i] & conf->frame.arp.smac.mask[i])) {
                    match = 0;
                }
            if (!match) {
                continue;
            }

            /* SIP/DIP */
            if ((sip & conf->frame.arp.sip.mask) !=
                (conf->frame.arp.sip.value & conf->frame.arp.sip.mask) ||
                (dip & conf->frame.arp.dip.mask) !=
                (conf->frame.arp.dip.value & conf->frame.arp.dip.mask)) {
                continue;
            }

            break;
        case MESA_ACE_TYPE_IPV4:
            /* IP flags */
            if (acl_flag_mismatch(conf, flags, ACE_FLAG_IP_TTL) ||
                acl_flag_mismatch(conf, flags, ACE_FLAG_IP_FRAGMENT) ||
                acl_flag_mismatch(conf, flags, ACE_FLAG_IP_OPTIONS)) {
                continue;
            }

            /* DS, proto, SIP and DIP */
            if ((ds & conf->frame.ipv4.ds.mask) !=
                (conf->frame.ipv4.ds.value & conf->frame.ipv4.ds.mask) ||
                (proto & conf->frame.ipv4.proto.mask) !=
                (conf->frame.ipv4.proto.value & conf->frame.ipv4.proto.mask) ||
                (sip & conf->frame.ipv4.sip.mask) !=
                (conf->frame.ipv4.sip.value & conf->frame.ipv4.sip.mask) ||
                (dip & conf->frame.ipv4.dip.mask) !=
                (conf->frame.ipv4.dip.value & conf->frame.ipv4.dip.mask)) {
                continue;
            }

            if (proto == 6 || proto == 17) {
                /* UDP/TCP port numbers */
                if (sport < conf->frame.ipv4.sport.low ||
                    sport > conf->frame.ipv4.sport.high ||
                    dport < conf->frame.ipv4.dport.low ||
                    dport > conf->frame.ipv4.dport.high) {
                    continue;
                }

                /* TCP flags */
                if (proto == 6 &&
                    (acl_flag_mismatch(conf, flags, ACE_FLAG_TCP_FIN) ||
                     acl_flag_mismatch(conf, flags, ACE_FLAG_TCP_SYN) ||
                     acl_flag_mismatch(conf, flags, ACE_FLAG_TCP_RST) ||
                     acl_flag_mismatch(conf, flags, ACE_FLAG_TCP_PSH) ||
                     acl_flag_mismatch(conf, flags, ACE_FLAG_TCP_ACK) ||
                     acl_flag_mismatch(conf, flags, ACE_FLAG_TCP_URG))) {
                    continue;
                }
            } else {
                /* IP data */
                for (i = 0, match = 1; i < 6; i++)
                    if ((p[i] & conf->frame.ipv4.data.mask[i]) !=
                        (conf->frame.ipv4.data.value[i] & conf->frame.ipv4.data.mask[i])) {
                        match = 0;
                    }
                if (!match) {
                    continue;
                }
            }
            break;

        case MESA_ACE_TYPE_IPV6: {
            /* DS, proto */
            if ((ds & conf->frame.ipv6.ds.mask) !=
                (conf->frame.ipv6.ds.value & conf->frame.ipv6.ds.mask) ||
                (proto & conf->frame.ipv6.proto.mask) !=
                (conf->frame.ipv6.proto.value & conf->frame.ipv6.proto.mask)) {
                continue;
            }

            /* SIPv6 */
            u32 v6_network = ((sip_v6.addr[12] & conf->frame.ipv6.sip.mask[12]) << 24) |
                             ((sip_v6.addr[13] & conf->frame.ipv6.sip.mask[13]) << 16) |
                             ((sip_v6.addr[14] & conf->frame.ipv6.sip.mask[14]) << 8) |
                             (sip_v6.addr[15] & conf->frame.ipv6.sip.mask[15]);
            if (v6_network !=
                (((conf->frame.ipv6.sip.value[12] & conf->frame.ipv6.sip.mask[12]) << 24) |
                 ((conf->frame.ipv6.sip.value[13] & conf->frame.ipv6.sip.mask[13]) << 16) |
                 ((conf->frame.ipv6.sip.value[14] & conf->frame.ipv6.sip.mask[14]) << 8) |
                 (conf->frame.ipv6.sip.value[15] & conf->frame.ipv6.sip.mask[15]))) {
                continue;
            }

            if (proto == 6 || proto == 17) {
                /* UDP/TCP port numbers */
                if (sport < conf->frame.ipv6.sport.low ||
                    sport > conf->frame.ipv6.sport.high ||
                    dport < conf->frame.ipv6.dport.low ||
                    dport > conf->frame.ipv6.dport.high) {
                    continue;
                }

                /* TCP flags */
                if (proto == 6 &&
                    (acl_flag_mismatch(conf, flags, ACE_FLAG_TCP_FIN) ||
                     acl_flag_mismatch(conf, flags, ACE_FLAG_TCP_SYN) ||
                     acl_flag_mismatch(conf, flags, ACE_FLAG_TCP_RST) ||
                     acl_flag_mismatch(conf, flags, ACE_FLAG_TCP_PSH) ||
                     acl_flag_mismatch(conf, flags, ACE_FLAG_TCP_ACK) ||
                     acl_flag_mismatch(conf, flags, ACE_FLAG_TCP_URG))) {
                    continue;
                }
            } else {
                /* IP data */
                for (i = 0, match = 1; i < 6; i++)
                    if ((p[i] & conf->frame.ipv6.data.mask[i]) !=
                        (conf->frame.ipv6.data.value[i] & conf->frame.ipv6.data.mask[i])) {
                        match = 0;
                    }
                if (!match) {
                    continue;
                }
            }
            break;
        }
        case MESA_ACE_TYPE_LLC:
        case MESA_ACE_TYPE_SNAP:
        default:
            /* These types are currently not used */
            continue;
        }

        /* ACE match ! */
        T_N("ACE ID %d match", conf->id);
        found_matched = TRUE;
        action = &conf->action;
        break;
    }

    /* Use port or ACE action */
    if (ace == NULL) {
        T_N("port %u match", port_no);
        action = &port_conf->action;
    }

    /* Logging */
    if (action->logging) {
        char *msg, buf0[40], buf1[40];

        msg = acl.log_buf;
        msg += sprintf(msg, "ACL-PKT_DUMPED: Frame of " VPRIz " bytes received on ", len);

#if defined(VTSS_SW_OPTION_SYSLOG)
        msg += sprintf(msg, "Interface %s\n", SYSLOG_PORT_INFO_REPLACE_KEYWORD);
#else
        msg += sprintf(msg, "port %u\n", iport2uport(port_no));
#endif /* VTSS_SW_OPTION_SYSLOG */

        msg += sprintf(msg,
                       "MAC:\n"
                       "  Destination: %s\n"
                       "  Source     : %s\n"
                       "  Type/Length: 0x%04x\n"
                       "  VLAN ID    : %d",
                       misc_mac_txt(dmac, buf0), misc_mac_txt(smac, buf1), etype, vid);

        switch (type) {
        case MESA_ACE_TYPE_IPV4:
        case MESA_ACE_TYPE_IPV6:
            if (type == MESA_ACE_TYPE_IPV6) {
                msg += sprintf(msg,
                               "\nIPv6:\n"
                               "  Protocol   : %d\n"
                               "  Source     : %s\n"
                               "  Destination: %s",
                               proto, misc_ipv6_txt(&sip_v6, buf0), misc_ipv6_txt(&dip_v6, buf1));
            } else {
                msg += sprintf(msg,
                               "\nIPv4:\n"
                               "  Protocol   : %d\n"
                               "  Source     : %s\n"
                               "  Destination: %s",
                               proto, misc_ipv4_txt(sip, buf0), misc_ipv4_txt(dip, buf1));
            }
            if (VTSS_BF_GET(flags, ACE_FLAG_IP_FRAGMENT) == 0) {
                switch (proto) {
                case 1:
                    msg += sprintf(msg,
                                   "\nICMP:\n"
                                   "  Type: %d\n"
                                   "  Code: %d",
                                   p[0], p[1]);
                    break;
                case 6:
                    msg += sprintf(msg,
                                   "\nTCP:\n"
                                   "  Source     : %d\n"
                                   "  Destination: %d\n"
                                   "  Flags      : %s%s%s%s%s%s",
                                   sport, dport,
                                   VTSS_BF_GET(flags, ACE_FLAG_TCP_FIN) ? "FIN " : "",
                                   VTSS_BF_GET(flags, ACE_FLAG_TCP_SYN) ? "SYN " : "",
                                   VTSS_BF_GET(flags, ACE_FLAG_TCP_RST) ? "RST " : "",
                                   VTSS_BF_GET(flags, ACE_FLAG_TCP_PSH) ? "PSH " : "",
                                   VTSS_BF_GET(flags, ACE_FLAG_TCP_ACK) ? "ACK " : "",
                                   VTSS_BF_GET(flags, ACE_FLAG_TCP_URG) ? "URG " : "");
                    break;
                case 17:
                    msg += sprintf(msg,
                                   "\nUDP\n"
                                   "  Source     : %d\n"
                                   "  Destination: %d ",
                                   sport, dport);
                    break;
                case 58:
                    msg += sprintf(msg,
                                   "\nICMPv6:\n"
                                   "  Type: %d\n"
                                   "  Code: %d",
                                   p[0], p[1]);
                    break;
                default:
                    break;
                }
            }
            break;
        case MESA_ACE_TYPE_ARP:
            if (VTSS_BF_GET(flags, ACE_FLAG_ARP_UNKNOWN) == 0) {
                msg += sprintf(msg,
                               "\n%sARP\n"
                               "  Opcode: Re%s\n"
                               "  Sender: %s\n"
                               "  Target: %s",
                               VTSS_BF_GET(flags, ACE_FLAG_ARP_ARP) ? "" : "R",
                               VTSS_BF_GET(flags, ACE_FLAG_ARP_REQ) ? "quest" : "ply",
                               misc_ipv4_txt(sip, buf0), misc_ipv4_txt(dip, buf1));
            }
            break;
        default:
            break;
        }

        msg += sprintf(msg, "\n\nFrame Dump:\n");
        for (i = 0; i < len && i < 1600; i++) {
            if ((n = (i % 16)) == 0) {
                msg += sprintf(msg, "%04X: ", i);
            }
            msg += sprintf(msg, "%02X%c", frame[i], n == 15 ? '\n' : n == 7 ? '-' : ' ');
        }
        msg[-1] = '\0';

#ifdef VTSS_SW_OPTION_SYSLOG
        S_PORT_I(VTSS_ISID_LOCAL, port_no, "%s", acl.log_buf);
#endif
        T_D_HEX(frame, len);
    }

    /* Port shut down */
    timer = &acl.port_shutdown_timer[port_no];
    if (action->shutdown && VTSS_MTIMER_TIMEOUT(timer)) {
        VTSS_MTIMER_START(timer, 1000);
        acl_stack_port_shutdown(port_no);
    }

    is_permit = action->port_action == MESA_ACL_PORT_ACTION_NONE ? 0 : 1;

    ACL_CRIT_EXIT();

    // Pass to other subscribers to receive the packet if the matched ACE is created by other ACL user
    if (found_matched && ace && ACL_USER_ID_GET(ace->conf.id) != ACL_USER_STATIC) {
        return FALSE;
    }

    return is_permit;
}
#endif /* VTSS_SW_OPTION_PACKET */

static void acl_packet_register(void)
{
#ifdef VTSS_SW_OPTION_PACKET
    packet_rx_filter_t filter;
    mesa_port_no_t     port_no;
    BOOL               reg;
    acl_action_t       *action;
    acl_ace_t          *ace;
    u32                port_count = port_count_max();

    /* Remove previous registration */
    ACL_LOCK_ASSERT_LOCKED("acl_packet_register");
    if (acl.filter_id != NULL) {
        if (packet_rx_filter_unregister(acl.filter_id) == VTSS_RC_OK) {
            acl.filter_id = NULL;
        }
    }

    /* Determine if registration is neccessary */
    reg = 0;
    for (port_no = 0; port_no < port_count; port_no++) {
        action = &acl.port_conf[port_no].action;
        if (action->logging || action->shutdown) {
            reg = 1;
        }
    }
    for (ace = acl.switch_acl.used; ace != NULL && ace->conf.conflict == FALSE; ace = ace->next) {
        action = &ace->conf.action;
        if (action->logging || action->shutdown) {
            reg = 1;
        }
    }

    if (reg) {
        packet_rx_filter_init(&filter);
        filter.prio  = PACKET_RX_FILTER_PRIO_HIGH;
        filter.modid = VTSS_MODULE_ID_ACL;
        filter.match = (PACKET_RX_FILTER_MATCH_ANY | PACKET_RX_FILTER_MATCH_SRC_PORT);
        for (port_no = 0; port_no < port_count; port_no++) {
            VTSS_PORT_BF_SET(filter.src_port_mask, port_no, 1);
        }
        filter.cb = acl_packet_rx;
        if (packet_rx_filter_register(&filter, &acl.filter_id) != VTSS_RC_OK) {
            T_W("ACL module register packet RX filter fail.");
        }
    }

#endif
}

/****************************************************************************/
/*  Management functions                                                    */
/****************************************************************************/

/* ACL error text */
const char *acl_error_txt(mesa_rc rc)
{
    const char *txt;

    switch (rc) {
    case VTSS_APPL_ACL_ERROR_GEN:
        txt = "ACL generic error";
        break;
    case VTSS_APPL_ACL_ERROR_ISID_NON_EXISTING:
        txt = "Switch ID is non-existing";
        break;
    case VTSS_APPL_ACL_ERROR_PARM:
        txt = "ACL parameter error";
        break;
    case VTSS_APPL_ACL_ERROR_STACK_STATE:
        txt = "ACL stack state error";
        break;
    case VTSS_APPL_ACL_ERROR_ACE_NOT_FOUND:
        txt = "ACE not found";
        break;
    case VTSS_APPL_ACL_ERROR_ACE_ALREDY_EXIST:
        txt = "ACE already existing";
        break;
    case VTSS_APPL_ACL_ERROR_ACE_TABLE_FULL:
        txt = "ACE table full";
        break;
    case VTSS_APPL_ACL_ERROR_USER_NOT_FOUND:
        txt = "ACL USER not found";
        break;
    case VTSS_APPL_ACL_ERROR_MEM_ALLOC_FAIL:
        txt = "Alloc memory fail";
        break;
    case VTSS_APPL_ACL_ERROR_ACE_AUTO_ASSIGNED_FAIL:
        txt = "ACE auto-assinged fail";
        break;
    case VTSS_APPL_ACL_ERROR_UNKNOWN_ACE_TYPE:
        txt = "Unknown ACE type";
        break;
    default:
        txt = "ACL unknown error";
        break;
    }
    return txt;
}

/* Determine if port and ISID are valid */
static BOOL acl_mgmt_port_sid_invalid( mesa_port_no_t port_no, BOOL set)
{

    if (port_no >= port_count_max()) {
        T_W("illegal port_no: %u", port_no);
        return 1;
    }

    return 0;
}

/* Get ACL port configuration */
mesa_rc acl_mgmt_port_conf_get(mesa_port_no_t port_no, acl_port_conf_t *conf)
{
    T_D("enter, port_no: %u", port_no);

    if (acl_mgmt_port_sid_invalid(port_no, 0)) {
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    ACL_CRIT_ENTER();
    *conf = acl.port_conf[port_no];
    ACL_CRIT_EXIT();
    T_D("exit");

    return VTSS_RC_OK;
}

/* Set ACL port configuration */
mesa_rc acl_mgmt_port_conf_set(mesa_port_no_t port_no, acl_port_conf_t *conf)
{
    mesa_rc         rc = VTSS_RC_OK;
    acl_port_conf_t *port_conf;

    T_D("enter, port_no: %u, policy: %u, policer: %u",
        port_no,
        conf->policy_no,
        conf->action.policer);

    if (acl_mgmt_port_sid_invalid(port_no, 1)) {
        T_D("exit: error port_sid");
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    ACL_CRIT_ENTER();
    port_conf = &acl.port_conf[port_no];
    if (acl_mgmt_port_conf_changed(port_conf, conf)) {
        *port_conf = *conf;
        /* Activate changed configuration */
        rc = acl_port_conf_set();
    }
    ACL_CRIT_EXIT();
    T_D("exit, port_no: %u", port_no);
    return rc;
}

/* Get ACL port counter */
mesa_rc acl_mgmt_port_counter_get(mesa_port_no_t port_no, mesa_acl_port_counter_t *const counter)
{
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter, port_no: %u", port_no);

    if (acl_mgmt_port_sid_invalid(port_no, 0)) {
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    *counter = 0;
    if (acl.def_port_ace_supported) {
        rc = mesa_ace_counter_get(NULL, ACL_DEF_PORT_ACE_ID_START + port_no, counter);
    } else {
        rc = mesa_acl_port_counter_get(NULL, port_no, counter);
    }
    return rc;
}

/* Get ACL policer configuration */
mesa_rc acl_mgmt_policer_conf_get(mesa_acl_policer_no_t policer_no,
                                  vtss_appl_acl_config_rate_limiter_t *conf)
{
    T_D("enter, policer_no: %u", policer_no);

    ACL_CRIT_ENTER();
    *conf = acl.policer_conf[policer_no];
    ACL_CRIT_EXIT();
    T_D("exit");

    return VTSS_RC_OK;
}

/* Set ACL policer configuration */
mesa_rc acl_mgmt_policer_conf_set(mesa_acl_policer_no_t policer_no,
                                  const vtss_appl_acl_config_rate_limiter_t *const conf)
{
    mesa_rc           rc = VTSS_RC_OK;
    int               changed;
    vtss_appl_acl_config_rate_limiter_t *pol_conf;

    T_D("enter, policer_no: %u, rate %d %s", policer_no, conf->bit_rate_enable ? conf->bit_rate : conf->packet_rate, conf->bit_rate_enable ? "kbps" : "pps");

    ACL_CRIT_ENTER();
    pol_conf = &acl.policer_conf[policer_no];
    changed = acl_mgmt_policer_conf_changed(pol_conf, conf);
    *pol_conf = *conf;
    if (pol_conf->bit_rate_enable) {
        pol_conf->packet_rate = 0;
    } else {
        pol_conf->bit_rate = 0;
    }

    if (changed) {
        /* Activate changed configuration */
        rc = acl_policer_conf_set();
    }
    ACL_CRIT_EXIT();
    T_D("exit, policer_no: %u", policer_no);
    return rc;
}

static BOOL acl_mgmt_isid_invalid(vtss_isid_t isid)
{
    if (isid > VTSS_ISID_END) {
        T_W("illegal isid: %d", isid);
        return 1;
    }

    if (isid != VTSS_ISID_LOCAL && !msg_switch_is_primary()) {
        T_W("not ready");
        return 1;
    }

    if (VTSS_ISID_LEGAL(isid) && !msg_switch_configurable(isid)) {
        T_W("isid %d not active", isid);
        return 1;
    }

    return 0;
}

/*lint -sem(acl_mgmt_ace_get, thread_protected)*/
/* Its safe to access global var 'ace_conf_get_flags' */
/* Get ACE or next ACE (use ACL_MGMT_ACE_ID_NONE to get first) */
mesa_rc acl_mgmt_ace_get(acl_user_t user_id,
                         mesa_ace_id_t id, acl_entry_conf_t *conf,
                         mesa_ace_counter_t *counter, BOOL next)
{
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter, user_id: %d \"%s\", id: %d, next: %d", user_id, acl_mgmt_user_name_get(user_id), id, next);

    /* Check user ID */
    if (user_id >= ACL_USER_CNT) {
        T_W("exit - user_id: %d not exist", user_id);
        return VTSS_APPL_ACL_ERROR_USER_NOT_FOUND;
    }

    /* Check ACE ID */
    if (id > ACL_MGMT_ACE_ID_END) {
        T_D("exit - id: %d out of range", id);
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    ACL_LOCK_SCOPE();
    rc = acl_list_ace_get(ACL_COMBINED_ID_SET(user_id, id), conf, counter, next);

    /* Restore independent ACE ID */
    conf->id = ACL_ACE_ID_GET(conf->id);

    T_D("exit");
    return rc;
}

/* Check ACL ACE configuration parameters */
static BOOL acl_ace_ipv4_params_invalid(acl_entry_conf_t *conf)
{
    /* The detailed parameters of IPv4 UDP/TCP/ICMP is not supported when IP Fragment or Options flag is set to 1. */
    if (conf->type == MESA_ACE_TYPE_IPV4 &&
        (conf->frame.ipv4.proto.value == 1 || conf->frame.ipv4.proto.value == 6 || conf->frame.ipv4.proto.value == 17) &&
        (acl_bit_get(ACE_FLAG_IP_FRAGMENT, conf) == MESA_ACE_BIT_1 || acl_bit_get(ACE_FLAG_IP_OPTIONS, conf) == MESA_ACE_BIT_1)) {
        BOOL                icmp_flag = FALSE, udp_tcp_port_flag = FALSE, tcp_flag = FALSE;
        mesa_ace_u48_t      default_ipv4_data;
        mesa_ace_udp_tcp_t  default_udp_tcp_port;

        if (conf->frame.ipv4.proto.value == 1) {
            memset(&default_ipv4_data, 0, sizeof(default_ipv4_data));
            icmp_flag = (memcmp(&conf->frame.ipv4.data, &default_ipv4_data, sizeof(default_ipv4_data))) ? TRUE : FALSE;
        } else {
            default_udp_tcp_port.in_range = 1;
            default_udp_tcp_port.low = 0;
            default_udp_tcp_port.high = 65535;
            udp_tcp_port_flag = (memcmp(&conf->frame.ipv4.sport, &default_udp_tcp_port, sizeof(default_udp_tcp_port)) ||
                                 memcmp(&conf->frame.ipv4.dport, &default_udp_tcp_port, sizeof(default_udp_tcp_port))) ? TRUE : FALSE;
            if (conf->frame.ipv4.proto.value == 6) {
                tcp_flag = (acl_bit_get(ACE_FLAG_TCP_FIN, conf) != MESA_ACE_BIT_ANY ||
                            acl_bit_get(ACE_FLAG_TCP_SYN, conf) != MESA_ACE_BIT_ANY ||
                            acl_bit_get(ACE_FLAG_TCP_RST, conf) != MESA_ACE_BIT_ANY ||
                            acl_bit_get(ACE_FLAG_TCP_PSH, conf) != MESA_ACE_BIT_ANY ||
                            acl_bit_get(ACE_FLAG_TCP_ACK, conf) != MESA_ACE_BIT_ANY ||
                            acl_bit_get(ACE_FLAG_TCP_URG, conf) != MESA_ACE_BIT_ANY) ? TRUE : FALSE;
            }
        }

        if (icmp_flag || udp_tcp_port_flag || tcp_flag) {
            return TRUE;
        }
    }

    return FALSE;
}

/* Add ACE entry before given ACE or last (ACL_MGMT_ACE_ID_NONE) */
mesa_rc acl_mgmt_ace_add(acl_user_t user_id, mesa_ace_id_t next_id, acl_entry_conf_t *conf)
{
    mesa_rc         rc = VTSS_RC_OK;
    T_D("enter, user_id: %d \"%s\", next_id: %d, conf->id: %d", user_id, acl_mgmt_user_name_get(user_id), next_id, conf->id);

    /* Check stack role */
    if (acl_mgmt_isid_invalid(conf->isid)) {
        T_D("exit");
        return VTSS_APPL_ACL_ERROR_STACK_STATE;
    }

    /* Check user ID */
    if (user_id >= ACL_USER_CNT) {
        T_W("user_id: %d not exist", user_id);
        T_D("exit");
        return VTSS_APPL_ACL_ERROR_USER_NOT_FOUND;
    }

    /* Check illegal parameter */
    if (next_id > ACL_MGMT_ACE_ID_END || conf->id > ACL_MGMT_ACE_ID_END) {
        T_W("exit VTSS_APPL_ACL_ERROR_PARM");
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    if (conf->action.port_action > MESA_ACL_PORT_ACTION_REDIR) {
        T_D("exit");
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    if (conf->type == MESA_ACE_TYPE_IPV4 && acl_ace_ipv4_params_invalid(conf)) {
        T_D("acl_ace_ipv4_params_invalid");
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    /* Force setting */
    if (user_id == ACL_USER_STATIC) {
        conf->action.force_cpu = 0;
        conf->action.cpu_once = 0;
    }

    /* Convert to combined ID */
    conf->id = ACL_COMBINED_ID_SET(user_id, conf->id);
    next_id = ACL_COMBINED_ID_SET(user_id, next_id);

    ACL_LOCK_SCOPE();
    if ((rc = acl_list_ace_add(&next_id, conf)) == VTSS_RC_OK) {
        /* Set this entry to ASIC layer */
        if (conf->new_allocated == FALSE && acl_ace_add(next_id, conf) == VTSS_RC_OK) {
            acl_packet_register();
        } else {
            conf->conflict = TRUE;
            acl_ace_conflict_set(conf->id, conf->conflict);
            if (user_id != ACL_USER_STATIC) {
                acl_local_ace_conflict_solve(TRUE);
            } else {
#if defined(VTSS_SW_OPTION_SYSLOG)
                S_N("ACL-CONFLICT: The %s ACE %d is not applied due to hardware limitations.",
                    acl_user_txt[ACL_USER_ID_GET(conf->id)],
                    ACL_ACE_ID_GET(conf->id));
#endif /* VTSS_SW_OPTION_SYSLOG */
                acl_msg_tx_event(TRUE);
            }
        }
    }

    /* Restore independent ACE ID */
    conf->id = ACL_ACE_ID_GET(conf->id);

    T_D("exit");
    return rc;
}

/* Solve conflict volatile ACE */
static void acl_local_ace_conflict_solve(BOOL new_volatile_ace_conflict)
{
    acl_list_t                  *list;
    acl_ace_t                   *ace, *prev, *new_ace;
    acl_ace_t                   *last_no_conflict_ace = NULL, *prev_last_no_conflict_ace = NULL;
    BOOL                        conflict_solved = FALSE, conflict_existing;
    mesa_ace_id_t               next_id;

    T_D("enter: new_volatile_ace_conflict = %s", new_volatile_ace_conflict ? "T" : "F");

    ACL_LOCK_ASSERT_LOCKED("acl_local_ace_conflict_solve");
    list = &acl.switch_acl;

    if (new_volatile_ace_conflict) {
        BOOL        found_conflict = FALSE;

        // Check if possible to solve conflict
        for (ace = list->used, prev = NULL, new_ace = NULL; ace != NULL; prev = ace, ace = ace->next) {
            if (!found_conflict && ace->conf.conflict) {
                found_conflict = TRUE;
            } else if (found_conflict && !ace->conf.conflict) {
                prev_last_no_conflict_ace = prev;
                last_no_conflict_ace = ace;
            }
        }

        if (!last_no_conflict_ace) {
            T_D("exit: No room for the new volatile ACE.");
            return;
        }

        // Delete the last no conflict entry
        if (acl_ace_del(last_no_conflict_ace->conf.id) == VTSS_RC_OK) {
            last_no_conflict_ace->conf.conflict = TRUE;
#if defined(VTSS_SW_OPTION_SYSLOG)
            S_N("ACL-CONFLICT: The %s ACE %d is not applied due to hardware limitations.",
                acl_user_txt[ACL_USER_ID_GET(last_no_conflict_ace->conf.id)],
                ACL_ACE_ID_GET(last_no_conflict_ace->conf.id));
#endif /* VTSS_SW_OPTION_SYSLOG */
        } else {
            T_D("exit: Calling acl_ace_del(%d) failed.", last_no_conflict_ace->conf.id);
            return;
        }
    }

    /* Lookup the first conflict entry, try to set it to ASIC layer again.
       If the setting is fail, try the next conflict entry. */
    for (ace = list->used, prev = NULL, new_ace = NULL; ace != NULL; prev = ace, ace = ace->next) {
        if (list->free && ace->conf.new_allocated) {
            /* Free the new alloc memory and change to use the list */
            new_ace = list->free;
            list->free = new_ace->next;
            new_ace->next = ace->next;
            new_ace->conf = ace->conf;
            new_ace->conf.new_allocated = FALSE;
            new_ace->conf.conflict = ace->conf.conflict;
            if (prev) {
                prev->next = new_ace;
            } else {
                list->used = new_ace;
            }
            VTSS_FREE(ace);
            ace = new_ace;
        }

        if (!conflict_solved && ace->conf.conflict) {
            /* Find the active next_id */
            next_id = ACL_MGMT_ACE_ID_NONE;
            while (ace->next && !ace->next->conf.conflict) {
                next_id = ace->next->conf.id;
                break;
            }

            /* Try to set this entry to ASIC layer again */
            if (acl_ace_add(next_id, &ace->conf) == VTSS_RC_OK) {
                ace->conf.conflict = FALSE;
                conflict_solved = TRUE;
                if (last_no_conflict_ace && new_volatile_ace_conflict && ace->conf.new_allocated) {
                    // Change new allocated memory as a conflict entry
                    acl_entry_conf_t temp_conf;
                    acl_ace_t        *temp_ace;

                    // Swap ACE configuration first
                    ace->conf.new_allocated = FALSE;
                    last_no_conflict_ace->conf.new_allocated = TRUE;
                    temp_conf = last_no_conflict_ace->conf;
                    last_no_conflict_ace->conf = ace->conf;
                    ace->conf = temp_conf;

                    // Then swap location in list
                    temp_ace = ace;

                    if (prev) {
                        prev->next = last_no_conflict_ace;
                    } else {
                        list->used = last_no_conflict_ace;
                    }
                    if (prev_last_no_conflict_ace) {
                        prev_last_no_conflict_ace->next = ace;
                    }

                    temp_ace = last_no_conflict_ace->next;
                    last_no_conflict_ace->next = ace->next;
                    ace->next = temp_ace;
                }
#if defined(VTSS_SW_OPTION_SYSLOG)
                S_N("ACL-CONFLICT: The conflict of %s ACE %d is solved.",
                    acl_user_txt[ACL_USER_ID_GET(ace->conf.id)],
                    ACL_ACE_ID_GET(ace->conf.id));
#endif /* VTSS_SW_OPTION_SYSLOG */
            }
        }
    }

    if (conflict_solved) {
        acl_packet_register();
    }

    /* Check if any conflicted entry existing */
    conflict_existing = FALSE;
    for (ace = list->used, prev = NULL, new_ace = NULL; ace != NULL; prev = ace, ace = ace->next) {
        if (ace->conf.conflict) {
            conflict_existing = TRUE;
            break;
        }
    }
    acl_msg_tx_event(conflict_existing);

    T_D("exit");
}

/* Delete ACE */
mesa_rc acl_mgmt_ace_del(acl_user_t user_id, mesa_ace_id_t id)
{
    mesa_rc     rc;
    BOOL        conflict = 1;

    T_D("enter, user_id: %d \"%s\", id: %d", user_id, acl_mgmt_user_name_get(user_id), id);

    /* Check user ID */
    if (user_id >= ACL_USER_CNT) {
        T_W("user_id: %d not exist", user_id);
        T_D("exit");
        return VTSS_APPL_ACL_ERROR_USER_NOT_FOUND;
    }

    /* Check ACE ID */
    if (id > ACL_MGMT_ACE_ID_END) {
        T_W("id: %d out of range", id);
        T_D("exit");
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    /* Convert to combined ID */
    id = ACL_COMBINED_ID_SET(user_id, id);

    T_D("Trying to delete id: %d", id);

    ACL_LOCK_SCOPE();

    if ((rc = acl_list_ace_del(id, &conflict)) == VTSS_RC_OK) {
        /* Remove this entry from ASIC layer */
        if (conflict == FALSE && acl_ace_del(id) != VTSS_RC_OK) {
            T_W("Calling acl_ace_del() failed\n");
        } else {
            acl_local_ace_conflict_solve(FALSE);
        }
    }

    T_D("exit");
    return rc;
}

mesa_rc acl_mgmt_counters_clear(void)
{
    T_D("enter");

    if (!msg_switch_is_primary()) {
        T_W("not ready");
        return VTSS_APPL_ACL_ERROR_STACK_STATE;
    }

    return acl_stack_clear();
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

/* Read/create ACL stack configuration */
static mesa_rc acl_conf_read(BOOL create)
{
    mesa_rc                         rc = VTSS_RC_OK;
    mesa_port_no_t                  port_no;
    acl_port_conf_t                 *port_conf, new_port_conf;
    mesa_acl_policer_no_t           policer_no;
    vtss_appl_acl_config_rate_limiter_t *pol_conf, new_pol_conf;
    int                             changed;
    u32                             port_count = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);
    u32                             pol_count = fast_cap(MESA_CAP_ACL_POLICER_CNT);

    T_D("enter, create: %d", create);
    ACL_LOCK_ASSERT_LOCKED("acl_conf_read");
    /* Port configuration */
    changed = 0;
    acl_mgmt_port_conf_get_default(&new_port_conf);
    for (port_no = 0; port_no < port_count; port_no++) {
        port_conf = &acl.port_conf[port_no];
        if (acl_mgmt_port_conf_changed(port_conf, &new_port_conf)) {
            changed = 1;
        } else if (acl.def_port_ace_supported && !acl.def_port_ace_init[port_no]) {
            changed = 1;
        }
        *port_conf = new_port_conf;
    }

    if (changed && create) {
        rc = acl_port_conf_set();
    }

    /* Policer configuration */
    changed = 0;
    acl_mgmt_policer_conf_get_default(&new_pol_conf);
    for (policer_no = 0; policer_no < pol_count; policer_no++) {
        pol_conf = &acl.policer_conf[policer_no];
        if (acl_mgmt_policer_conf_changed(pol_conf, &new_pol_conf)) {
            changed = 1;
        }
        *pol_conf = new_pol_conf;
    }

    if (changed && create) {
        if (acl_policer_conf_set() != VTSS_RC_OK) {
            T_W("acl_policer_conf_set() failed\n");
        }
    }

    if (create) {
        rc = acl_ace_conf_set();
    }

    T_D("exit");
    return rc;
}

#if defined(ACL_PACKET_TEST)
static BOOL acl_packet_etype_rx(void *contxt, const uchar *const frame, const mesa_packet_rx_info_t *const rx_info)
{
    return 1; /* Consume frame */
}

static void acl_packet_etype_register(void)
{
    packet_rx_filter_t filter;
    void               *filter_id;

    packet_rx_filter_init(&filter);
    filter.prio  = (PACKET_RX_FILTER_PRIO_SUPER - 1);
    filter.modid = VTSS_MODULE_ID_ACL;
    filter.match = PACKET_RX_FILTER_MATCH_ETYPE;
    filter.etype = 0xaaaa;
    filter.cb = acl_packet_etype_rx;
    if (packet_rx_filter_register(&filter, &filter_id) != VTSS_RC_OK) {
        T_W("ACL module register etype packet RX filter fail.");
    }
}
#endif /* ACL_PACKET_TEST */

const char *acl_mgmt_user_name_get(
    acl_user_t user_id
)
{
    if (user_id >= ACL_USER_CNT) {
        return NULL;
    }
    return acl_user_txt[user_id];
}

/**
  * \brief Get ACL user ID by name
  *
  * \param user_name [IN]: ACL user name.
  *
  * \return
  *    acl_user_t - ACL user ID.
  */
acl_user_t acl_mgmt_user_id_get_by_name(
    char *user_name)
{
    if (!user_name) {
        return ACL_USER_CNT;
    }
    for (int i = 0; i < ACL_USER_CNT; i++) {
        if (strcmp(user_name, acl_user_txt[i]) == 0) {
            return (acl_user_t)i;
        }
    }
    return ACL_USER_CNT;
}

/* Initialize ACE to default values (permit on all front ports) */
mesa_rc acl_mgmt_ace_init(mesa_ace_type_t type, acl_entry_conf_t *ace)
{
    mesa_port_no_t iport;
    u32            port_count = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    vtss_clear(*ace);
    ace->id = ACL_MGMT_ACE_ID_NONE;
    ace->type = type;
    ace->isid = VTSS_ISID_GLOBAL;
    ace->action.policer = ACL_MGMT_RATE_LIMITER_NONE;
    ace->action.cpu_queue = PACKET_XTR_QU_ACL_COPY; /* Default ACL queue */
    ace->action.port_action = MESA_ACL_PORT_ACTION_NONE;
    for (iport = 0; iport < port_count; iport++) {
        ace->port_list[iport] = TRUE;
        ace->action.port_list[iport] = TRUE;
    }

    switch (type) {
    case MESA_ACE_TYPE_ANY:
    case MESA_ACE_TYPE_ETYPE:
    case MESA_ACE_TYPE_LLC:
    case MESA_ACE_TYPE_SNAP:
    case MESA_ACE_TYPE_ARP:
        break;
    case MESA_ACE_TYPE_IPV4:
        ace->frame.ipv4.sport.in_range = ace->frame.ipv4.dport.in_range = 1;
        ace->frame.ipv4.sport.high = ace->frame.ipv4.dport.high = 65535;
        break;
    case MESA_ACE_TYPE_IPV6:
        ace->frame.ipv6.sport.in_range = ace->frame.ipv6.dport.in_range = 1;
        ace->frame.ipv6.sport.high = ace->frame.ipv6.dport.high = 65535;
        break;
    default:
        T_E("unknown type: %d", type);
        T_D("Exit");
        return VTSS_APPL_ACL_ERROR_UNKNOWN_ACE_TYPE;
    }

    return VTSS_RC_OK;
}

/* Module start */
static void acl_start(BOOL init)
{
    int                   i;
    acl_ace_t             *ace;
    acl_list_t            *list;
    mesa_port_no_t        port_no;
    mesa_acl_policer_no_t policer_no;
    u32                   port_count = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);
    u32                   pol_count = fast_cap(MESA_CAP_ACL_POLICER_CNT);
    T_D("enter, init: %d", init);

    if (init) {
        for (i = 0; i < ACL_USER_CNT ; ++i) {
            if (strlen(acl_user_txt[i]) == 0) {
                T_E("ACL user name MUST not null string in acl_user_txt[%d]", i);
            }
        }

        /* Initialize port configuration */
        for (port_no = 0; port_no < port_count; port_no++) {
            acl_mgmt_port_conf_get_default(&acl.port_conf[port_no]);
            VTSS_MTIMER_START(&acl.port_shutdown_timer[port_no], 0);
        }

        /* Initialize policer configuration */
        for (policer_no = 0; policer_no < pol_count; policer_no++) {
            acl_mgmt_policer_conf_get_default(&acl.policer_conf[policer_no]);
        }

        /* Initialize ACL for switch: All free */
        list = &acl.switch_acl;
        list->used = NULL;
        list->free = NULL;
        for (i = 0; i < ACL_MGMT_ACE_MAX; i++) {
            ace = &acl.switch_ace_table[i];
            ace->next = list->free;
            list->free = ace;
        }

        acl.filter_id = NULL;

        /* Create semaphore for critical regions */
        critd_init(&acl.crit,       "acl",       VTSS_MODULE_ID_ACL, CRITD_TYPE_MUTEX);

#ifdef VTSS_SW_OPTION_IP
        critd_init(&acl_ifmux_crit, "acl_ifmux", VTSS_MODULE_ID_ACL, CRITD_TYPE_MUTEX);
#endif

        if (fast_cap(MESA_CAP_MISC_CHIP_FAMILY) == MESA_CHIP_FAMILY_CARACAL) {
            acl.ifmux_supported = TRUE;
            acl.def_port_ace_supported = TRUE;
        }
    } else {
#if defined(ACL_PACKET_TEST)
        acl_packet_etype_register();
#endif /* ACL_PACKET_TEST */
    }
    T_D("exit");
}

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
/* Initialize private mib */
VTSS_PRE_DECLS void acl_mib_init(void);
#endif

#if defined(VTSS_SW_OPTION_JSON_RPC)
VTSS_PRE_DECLS void vtss_appl_acl_json_init(void);
#endif
extern "C" int acl_icli_cmd_register();

/* Initialize module */
mesa_rc acl_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;

    T_D("enter, cmd: %d, isid: %u, flags: 0x%x", data->cmd, data->isid, data->flags);

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");
        acl_start(1);
#ifdef VTSS_SW_OPTION_ICFG
        if (acl_icfg_init() != VTSS_RC_OK) {
            T_W("Calling acl_icfg_init() failed");
        }
#endif

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
        /* Register private mib */
        acl_mib_init();
#endif

#if defined(VTSS_SW_OPTION_JSON_RPC)
        vtss_appl_acl_json_init();
#endif
        acl_icli_cmd_register();
        break;

    case INIT_CMD_START:
        T_D("START");
        acl_start(0);
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);

        // In case acl shutdown a port, it his responsibility to restore it
        acl_stack_port_restore();

        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
#ifdef VTSS_SW_OPTION_IP
            if (acl.ifmux_supported) {
                mesa_rc  rc;
                uint32_t cnt;

                ACL_CRIT_ENTER();
                if ((rc = rule_del(&ACL_ifmux.owner, &cnt)) == VTSS_RC_OK) {
                    ACL_ifmux.reset_data();
                } else {
                    T_W("rule_del() failed, cnt = %u\n", cnt);
                }
                acl.def_port_ace_init.clear();
                ACL_CRIT_EXIT();
            }
#endif
        } else if (isid == VTSS_ISID_GLOBAL) {
            /* Reset stack configuration */
            ACL_CRIT_ENTER();
            if (acl_conf_read(1) != VTSS_RC_OK) {
                T_W("acl_conf_read() failed");
            }
            ACL_CRIT_EXIT();
        }
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");
        {
            ACL_LOCK_SCOPE();
            /* Read stack and switch configuration */
            if (acl_conf_read(0) != VTSS_RC_OK) {
                T_W("acl_conf_read() failed");
            }
            /* Apply all configuration to switch */
            if (acl_port_conf_set() != VTSS_RC_OK) {
                T_W("acl_port_conf_set() failed");
            }
            if (acl_policer_conf_set() != VTSS_RC_OK) {
                T_W("acl_policer_conf_set() failed");
            }
            if (acl_ace_conf_set() != VTSS_RC_OK) {
                T_W("acl_ace_conf_set() failed");
            }

        }
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_D("ICFG_LOADING_POST");
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

#ifdef VTSS_SW_OPTION_IP
/*
==============================================================================
    Bugzilla#19161 - Filtering of ACL-copy-to-cpu frames

    Under Linux the frame-flow has been changed such that frames are received in
    the kernel, and then injected into the application and the IP stack at the
    same time. This means that the application can not consume frames and thereby
    prevent them from being injected into the IP stack.

    To solve this issue on Linux, the deny ACEs need to apply the corresponding
    deny-list rules via vtss-if-mux driver (documented in
    vtss_appl/ip/ip_filter_api.hxx)
==============================================================================
*/

/* Update ifmux bitmask from hardware index */
static mesa_rc acl_ifmux_update_hw_bitmask(mesa_ace_id_t ace_id, Bitmask *bitmask, BOOL is_set)
{
    mesa_rc           rc;
    u32               idx, offset;
    mesa_ace_status_t hw_status;

    if ((rc = mesa_ace_status_get(NULL, ace_id, &hw_status)) == VTSS_RC_OK) {
        for (idx = 0; idx < 2; ++idx) {
            T_D("ace_id = %u, hw_status.idx[idx] = %d\n", ace_id, hw_status.idx[idx]);
            if (hw_status.idx[idx] == MESA_ACE_IDX_NONE || hw_status.idx[idx] >= (ACL_MGMT_ACE_MAX - 1)) {
                continue;
            }
            offset = ACL_BITMASK_ARRAY_OFFSET(hw_status.idx[idx]);
            if (offset > ACL_BITMASK_MAX_OFFSET) {
                T_W("bitmask offset overflow, offset = %d, hw_status.idx[%d] = %d", offset, idx, hw_status.idx[idx]);
                rc = VTSS_RC_ERROR;
            } else {
                T_D("bitmsk array offset = %d", offset);
                bitmask[offset].offset = offset;
                VTSS_BF_SET(bitmask[offset].buf, (hw_status.idx[idx] - (offset * ACL_BITMASK_BITS_CNT)), is_set ? 1 : 0);
            }
        }
    }

    return rc;
}
#endif

#ifdef VTSS_SW_OPTION_IP
/* Add new ACE ID into ifmux rule */
static mesa_rc acl_ifmux_add(mesa_ace_id_t new_id)
{
    mesa_rc  rc = VTSS_RC_OK;
    Rule     rule;
    Bitmask  temp_bitmask[ACL_BITMASK_ARRAY_SIZE] = {};
    BOOL     rule_not_exist, found_new_id = FALSE;

    T_D("Enter: new_id=%u", new_id);
    ACL_IFMUX_CRIT_ENTER();

    // Lookup if the new ACE ID existing
    if ((rule_not_exist = !ACL_ifmux.ifmux_id) == TRUE) {
        found_new_id = ACL_ifmux.ace_lookup(&new_id, FALSE);
        T_D("%sFound ace_id=%u", found_new_id ? "" : "Not ", new_id);
    }

    if (!found_new_id) {
        if (!rule_not_exist) {
            memcpy(temp_bitmask, ACL_ifmux.bitmask, sizeof(temp_bitmask));
        }

        rc = acl_ifmux_update_hw_bitmask(new_id, temp_bitmask, TRUE);

        if (rc == VTSS_RC_OK) {
            Rule rule;

            // Generate rule according to the new bitmask
            ACL_ifmux.gen_rule(temp_bitmask, rule);

            if (rule_not_exist) {
                if ((rc = deny_list_rule_add(&ACL_ifmux.ifmux_id, &ACL_ifmux.owner, rule, Action::drop)) == VTSS_RC_OK) {
                    T_D("Calling deny_list_rule_add(ifmux_id=%d) success", ACL_ifmux.ifmux_id);
                } else {
                    T_W("Calling deny_list_rule_add() failed, rc=%u", rc);
                }
            } else {
                if ((rc = rule_update(ACL_ifmux.ifmux_id, rule)) == VTSS_RC_OK) {
                    T_D("Calling rule_update(ifmux_id=%d) success", ACL_ifmux.ifmux_id);
                } else {
                    T_W("Calling rule_update(ifmux_id=%d) failed, rc=%u", ACL_ifmux.ifmux_id, rc);
                }
            }

            if (rc == VTSS_RC_OK) {
                ACL_ifmux.ace_id.emplace_back(new_id);
                memcpy(ACL_ifmux.bitmask, temp_bitmask, sizeof(temp_bitmask));
            }
        }
    }

    ACL_IFMUX_CRIT_EXIT();
    T_D("Exit: rc = %u", rc);

    return rc;
}
#endif

#ifdef VTSS_SW_OPTION_IP
/* Delete ACE ID from ifmux rule */
static mesa_rc acl_ifmux_del(mesa_ace_id_t del_id)
{
    mesa_rc  rc = VTSS_RC_OK;
    BOOL found_del_id = FALSE, clean_rule = FALSE;

    T_D("Enter: ace_id=%u", del_id);
    ACL_IFMUX_CRIT_ENTER();

    // Lookup if ifmux_ace existing
    if ((found_del_id = ACL_ifmux.ace_lookup(&del_id, FALSE)) == TRUE) {
        clean_rule = ACL_ifmux.ace_id.size() == 1 ? TRUE : FALSE; // last one, delete ifmux rule
    }

    if (clean_rule) {
        if ((rc = rule_del(ACL_ifmux.ifmux_id)) == VTSS_RC_OK) {
            T_D("Calling rule_del(ifmux_id=%d) success", ACL_ifmux.ifmux_id);
            ACL_ifmux.reset_data();
        } else {
            T_W("Calling rule_del(ifmux_id=%d) failed, rc=%u", ACL_ifmux.ifmux_id, rc);
        }
    } else if (found_del_id) {
        Bitmask temp_bitmask[ACL_BITMASK_ARRAY_SIZE];

        // Update bitmask
        memcpy(temp_bitmask, ACL_ifmux.bitmask, sizeof(temp_bitmask));

        rc = acl_ifmux_update_hw_bitmask(del_id, temp_bitmask, FALSE);

        if (rc == VTSS_RC_OK) {
            Rule    rule;

            // Generate rule according to the new bitmask
            ACL_ifmux.gen_rule(temp_bitmask, rule);

            if ((rc = rule_update(ACL_ifmux.ifmux_id, rule)) == VTSS_RC_OK) {
                T_D("Calling rule_update(ifmux_id=%d) success", ACL_ifmux.ifmux_id);
                ACL_ifmux.ace_id.erase(remove(ACL_ifmux.ace_id.begin(), ACL_ifmux.ace_id.end(), del_id), ACL_ifmux.ace_id.end());
                memcpy(ACL_ifmux.bitmask, temp_bitmask, sizeof(temp_bitmask));
            } else {
                T_W("Calling rule_update(ifmux_id=%d) failed, rc=%u", ACL_ifmux.ifmux_id, rc);
            }
        }
    }

    ACL_IFMUX_CRIT_EXIT();
    T_D("Exit: rc = %u", rc);

    return rc;
}
#endif

#ifdef VTSS_SW_OPTION_IP
/* Update ifmux rule
 * Whenever an ACE change (add/delete) has been done through the VTSS
 * ACL API (for an ACL rule or a default port rule), IFH.ACL_IDX may
 * be changed. The ACL module must go through all rules again.
 */
static mesa_rc acl_ifmux_update(void)
{
    mesa_rc           rc = VTSS_RC_OK;
    mesa_ace_status_t hw_status;
    Rule              rule;
    Bitmask           temp_bitmask[ACL_BITMASK_ARRAY_SIZE] = {};

    T_D("Enter");
    ACL_IFMUX_CRIT_ENTER();

    // If only one element is existing since the data is correct
    // when it is applied. So we can ignore the process.
    if (ACL_ifmux.ace_id.size() > 1) {
        Vector<mesa_ace_id_t>::iterator iter = ACL_ifmux.ace_id.begin();
        for (; iter != ACL_ifmux.ace_id.end(); ++iter) {
            if (mesa_ace_status_get(NULL, *iter, &hw_status) != VTSS_RC_OK) {
                continue;
            }

            (void) acl_ifmux_update_hw_bitmask(*iter, temp_bitmask, TRUE);
        }

        // Generate rule according to the new bitmask
        ACL_ifmux.gen_rule(temp_bitmask, rule);

        if ((rc = rule_update(ACL_ifmux.ifmux_id, rule)) == VTSS_RC_OK) {
            memcpy(ACL_ifmux.bitmask, temp_bitmask, sizeof(temp_bitmask));
        } else {
            T_W("Calling rule_update(ifmux_id=%d) failed, rc=%u", ACL_ifmux.ifmux_id, rc);
        }
    }

    ACL_IFMUX_CRIT_EXIT();
    T_D("exit, rc = %u", rc);
    return rc;
}
#endif

#ifdef VTSS_SW_OPTION_IP
/* Get/Getnext ACE interface mux rule */
/**
 * \brief Get/Getnext ACE interface mux rule.
 *
 * \param id      [IN]    Indentify the ACE ID.
 *
 * \param counter [OUT]   The counter of the specific ACE. It can be equal
 *                        NULL if you don't need the information.
 *
 * \param next    [IN]    Indentify if it is getnext operation.
 * \return
 *    VTSS_RC_OK on success.\n
 *    VTSS_APPL_ACL_ERROR_ACE_NOT_FOUND if the ACE ID not found.\n
 **/
mesa_rc acl_mgmt_ifmux_get(mesa_ace_id_t *ace_id, int *ifmux_id, BOOL next)
{
    mesa_rc rc = VTSS_APPL_ACL_ERROR_ACE_NOT_FOUND;
    BOOL found = FALSE;

    ACL_IFMUX_CRIT_ENTER();
    found = ACL_ifmux.ace_lookup(ace_id, next);

    if (found) {
        *ifmux_id = ACL_ifmux.ifmux_id;
        rc = VTSS_RC_OK;
    }
    ACL_IFMUX_CRIT_EXIT();

    return rc;
}
#endif

/*
==============================================================================

    Public APIs in vtss_appl\include\vtss\appl\acl.h

==============================================================================
*/
#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef INOUT
#define INOUT
#endif

static BOOL _index_bit_get(
    IN  vtss_isid_t     isid,
    IN  mesa_port_no_t  iport,
    OUT u32             *index,
    OUT u32             *bit
)
{
    /* The port mapping of vtss_port_list_stackable_t is refer to
     * <TOP_DIR>\vtss_appl\util\vtss_appl_types.cxx/PortListStackable::isid_port_to_index()
     */
    vtss_uport_no_t     uport;
    u32                 i;

    uport = iport2uport(iport);

    i = (isid - 1) * 8 + uport / 8;
    if (i > 127) {
        T_W("invalid index %u", i);
        return FALSE;
    }

    if (index) {
        *index = i;
    }

    if (bit) {
        *bit = uport % 8;
    }

    return TRUE;
}

static BOOL _port_list_stackable_get(
    IN vtss_isid_t                         isid,
    IN mesa_port_no_t                      iport,
    IN const vtss_port_list_stackable_t    *const p
)
{
    u32     index;
    u32     bit;

    if (_index_bit_get(isid, iport, &index, &bit) == FALSE) {
        return FALSE;
    }
    return (p->data[index] >> bit) & 0x01;
}

static BOOL _port_list_stackable_set(
    IN  vtss_isid_t                     isid,
    IN  mesa_port_no_t                  iport,
    OUT vtss_port_list_stackable_t      *p
)
{
    u32     index;
    u32     bit;

    if (_index_bit_get(isid, iport, &index, &bit) == FALSE) {
        return FALSE;
    }

    p->data[index] |= (1 << bit);

    return TRUE;
}

#if 0 /* not used */
static BOOL _port_list_stackable_clear(
    IN  vtss_isid_t                     isid,
    IN  mesa_port_no_t                  iport,
    OUT vtss_port_list_stackable_t      *p
)
{
    if (_index_bit_get(isid, iport, &index, &bit) == FALSE) {
        return FALSE;
    }

    p->data[index] &= (~(1 << bit));

    return TRUE;
}
#endif

/* Convert mesa_ace_bit_t to vtss_appl_acl_ace_vlan_tagged_t */
static vtss_appl_acl_ace_vlan_tagged_t _vcap_tagged_type2value(mesa_ace_bit_t vcap_tagged)
{
    return (vtss_appl_acl_ace_vlan_tagged_t) vcap_tagged;
}

/* Convert vtss_appl_acl_ace_vlan_tagged_t to mesa_ace_bit_t */
static mesa_rc _vcap_tagged_value2type(vtss_appl_acl_ace_vlan_tagged_t tagged_value, mesa_ace_bit_t *vcap_tagged)
{
    if (vcap_tagged == NULL ||
        tagged_value >= VTSS_APPL_ACL_ACE_VLAN_TAGGED_CNT) {
        return VTSS_APPL_ACL_ERROR_PARM;
    }
    *vcap_tagged = (mesa_ace_bit_t) tagged_value;

    return VTSS_RC_OK;
}

/* Export ACL ACE dmac type */
static vtss_appl_vcap_adv_dmac_type_t   _dmac_export(
    vtss_appl_acl_ace_frame_type_t  frame_type,
    acl_entry_conf_t                *const ace
)
{
    vtss_appl_vcap_adv_dmac_type_t      dmac_type = VTSS_APPL_VCAP_ADV_DMAC_TYPE_ANY;
    acl_flag_t                          bc;
    acl_flag_t                          mc;

    if (frame_type == (vtss_appl_acl_ace_frame_type_t) ace->type) {
        bc = VTSS_BF_GET(ace->flags.mask, ACE_FLAG_DMAC_BC) ? (VTSS_BF_GET(ace->flags.value, ACE_FLAG_DMAC_BC) ? MESA_ACE_BIT_1 : MESA_ACE_BIT_0) : MESA_ACE_BIT_ANY;
        mc = VTSS_BF_GET(ace->flags.mask, ACE_FLAG_DMAC_MC) ? (VTSS_BF_GET(ace->flags.value, ACE_FLAG_DMAC_MC) ? MESA_ACE_BIT_1 : MESA_ACE_BIT_0) : MESA_ACE_BIT_ANY;
        if (bc == MESA_ACE_BIT_ANY && mc == MESA_ACE_BIT_ANY) { // Any
            dmac_type = VTSS_APPL_VCAP_ADV_DMAC_TYPE_ANY;
        } else if (bc == MESA_ACE_BIT_0 && mc == MESA_ACE_BIT_1) { // MC
            dmac_type = VTSS_APPL_VCAP_ADV_DMAC_TYPE_MULTICAST;
        } else if (bc == MESA_ACE_BIT_1 && mc == MESA_ACE_BIT_1) { // BC
            dmac_type = VTSS_APPL_VCAP_ADV_DMAC_TYPE_BROADCAST;
        } else if (bc == MESA_ACE_BIT_0 && mc == MESA_ACE_BIT_0) { // UC
            dmac_type = VTSS_APPL_VCAP_ADV_DMAC_TYPE_UNICAST;
        }
    }

    return dmac_type;
}

/* Import ACL ACE dmac type */
static mesa_rc _dmac_import(
    vtss_appl_vcap_adv_dmac_type_t  dmac_type,
    acl_entry_conf_t                *const ace
)
{
    if (dmac_type >= VTSS_APPL_VCAP_ADV_DMAC_TYPE_CNT) {
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    if (dmac_type == VTSS_APPL_VCAP_ADV_DMAC_TYPE_MULTICAST) {
        VTSS_BF_SET(ace->flags.mask, ACE_FLAG_DMAC_MC, 1);
        VTSS_BF_SET(ace->flags.value, ACE_FLAG_DMAC_MC, 1);
        VTSS_BF_SET(ace->flags.mask, ACE_FLAG_DMAC_BC, 1);
        VTSS_BF_SET(ace->flags.value, ACE_FLAG_DMAC_BC, 0);
    } else if (dmac_type == VTSS_APPL_VCAP_ADV_DMAC_TYPE_BROADCAST) {
        VTSS_BF_SET(ace->flags.mask, ACE_FLAG_DMAC_MC, 1);
        VTSS_BF_SET(ace->flags.value, ACE_FLAG_DMAC_MC, 1);
        VTSS_BF_SET(ace->flags.mask, ACE_FLAG_DMAC_BC, 1);
        VTSS_BF_SET(ace->flags.value, ACE_FLAG_DMAC_BC, 1);
    } else if (dmac_type == VTSS_APPL_VCAP_ADV_DMAC_TYPE_UNICAST) {
        VTSS_BF_SET(ace->flags.mask, ACE_FLAG_DMAC_MC, 1);
        VTSS_BF_SET(ace->flags.value, ACE_FLAG_DMAC_MC, 0);
        VTSS_BF_SET(ace->flags.mask, ACE_FLAG_DMAC_BC, 1);
        VTSS_BF_SET(ace->flags.value, ACE_FLAG_DMAC_BC, 0);
    } else { // any
        VTSS_BF_SET(ace->flags.mask, ACE_FLAG_DMAC_MC, 0);
        VTSS_BF_SET(ace->flags.value, ACE_FLAG_DMAC_MC, 0);
        VTSS_BF_SET(ace->flags.mask, ACE_FLAG_DMAC_BC, 0);
        VTSS_BF_SET(ace->flags.value, ACE_FLAG_DMAC_BC, 0);
    }

    return VTSS_RC_OK;
}

/* Export ACL ACE flags */
static mesa_vcap_bit_t _flag_export(acl_flag_t acl_flag, const acl_entry_conf_t *const ace)
{
    return (VTSS_BF_GET(ace->flags.mask, acl_flag) ? (VTSS_BF_GET(ace->flags.value, acl_flag) ? MESA_VCAP_BIT_1 : MESA_VCAP_BIT_0) : MESA_VCAP_BIT_ANY);
}

/* Import ACL ACE flags */
static void _flag_import(mesa_vcap_bit_t bit_flag, acl_flag_t acl_flag, acl_entry_conf_t *const ace)
{
    if (bit_flag == MESA_VCAP_BIT_0) {
        VTSS_BF_SET(ace->flags.mask, acl_flag, 1);
        VTSS_BF_SET(ace->flags.value, acl_flag, 0);
    } else if (bit_flag == MESA_VCAP_BIT_1) {
        VTSS_BF_SET(ace->flags.mask, acl_flag, 1);
        VTSS_BF_SET(ace->flags.value, acl_flag, 1);
    } else { // any
        VTSS_BF_SET(ace->flags.mask, acl_flag, 0);
        VTSS_BF_SET(ace->flags.value, acl_flag, 0);
    }
}

static BOOL _no_port_configured(
    IN const vtss_port_list_stackable_t     *const p
)
{
    mesa_port_no_t iport;
    u32            port_count = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    for (iport = 0; iport < port_count; iport++) {
        if (_port_list_stackable_get(VTSS_ISID_START, iport, p)) {
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL _stackable_set_by_port_list(
    IN  vtss_isid_t                     ace_isid,
    IN  mesa_port_list_t                &port_list,
    OUT vtss_port_list_stackable_t      *stacklist
)
{
    vtss_isid_t    isid;
    vtss_isid_t    isid_start;
    vtss_isid_t    isid_end;
    mesa_port_no_t iport;
    u32            port_count = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    if (ace_isid == VTSS_ISID_GLOBAL) {
        isid_start = VTSS_ISID_START;
        isid_end   = VTSS_ISID_END;
    } else {
        isid_start = ace_isid;
        isid_end   = ace_isid + 1;
    }

    for (isid = isid_start; isid < isid_end; ++isid) {
        for (iport = 0; iport < port_count; iport++) {
            if (port_list[iport]) {
                if (_port_list_stackable_set(isid, iport, stacklist) == FALSE) {
                    T_W("_port_list_stackable_set(%u, %u)", isid, iport);
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

/*
    Current design allows All ISIDs or single isid.
    Otherwise, FALSE
*/
static BOOL _port_list_get_by_stackable(
    IN  const vtss_port_list_stackable_t    *stacklist,
    OUT vtss_isid_t                         *ace_isid,
    OUT mesa_port_list_t                    &port_list
)
{
    vtss_isid_t      isid;
    vtss_isid_t      single_isid;
    mesa_port_no_t   iport;
    mesa_port_list_t list;
    BOOL             b_set;
    BOOL             b_all_isid;
    u32             port_count = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    /* get list from the first set isid */
    memset(list, 0, sizeof(list));

    b_set = FALSE;
    isid  = VTSS_ISID_START;
    for (iport = 0; iport < port_count; iport++) {
        if (_port_list_stackable_get(isid, iport, stacklist)) {
            b_set = TRUE;
            list[iport] = TRUE;
        }
    }

    b_all_isid = FALSE;

    if (b_set) {
        // check if all ISIDs
        isid  = VTSS_ISID_START + 1;
        for (iport = 0; iport < port_count; iport++) {
            if (_port_list_stackable_get(isid, iport, stacklist)) {
                b_all_isid = TRUE;
                break;
            }
        }

        if (!b_all_isid) {
            single_isid = VTSS_ISID_START;
        }
    } else {
        // must be single isid
        single_isid = 0;
    }

    if (ace_isid) {
        if (b_all_isid) {
            *ace_isid = VTSS_ISID_GLOBAL;
        } else {
            *ace_isid = single_isid;
        }
    }

    port_list = list;

    return TRUE;
}

/* Check ACL ACE action parameters */
static BOOL _ace_action_params_invalid(
    IN const vtss_appl_acl_action_t     *const ace_action
)
{
    // policer_id
    if (ace_action->rate_limiter_id > ACL_MGMT_RATE_LIMITER_NO_END) {
        T_D("ACL policer_id out of range");
        return TRUE;
    }

    return FALSE;
}

/* Export ACL ACE application action configuration */
static mesa_rc _ace_action_export(
    IN  const acl_action_t      *const action,
    OUT vtss_appl_acl_action_t  *const ace_action
)
{
    mesa_rc rc = VTSS_APPL_ACL_ERROR_PARM;

    /* Check illegal parameters */
    if (action == NULL || ace_action == NULL) {
        T_E("Input parameter is NULL");
        return rc;
    }

    /* Clear application configuration database */
    memset(ace_action, 0, sizeof(*ace_action));

    // policer
    ace_action->rate_limiter_id = action->policer == ACL_MGMT_RATE_LIMITER_NONE ? VTSS_APPL_ACL_POLICER_DISABLED : ipolicer2upolicer(action->policer);

    // Mirror
    ace_action->mirror = action->mirror;

    // Logging
    ace_action->logging = action->logging;

    // Shutdown
    ace_action->shutdown = action->shutdown;

    return VTSS_RC_OK;
}

/* Import ACL ACE application action configuration */
static mesa_rc _ace_action_import(
    IN  const vtss_appl_acl_action_t    *const ace_action,
    OUT acl_action_t                    *const action
)
{
    mesa_rc rc = VTSS_APPL_ACL_ERROR_PARM;

    T_D("enter");

    /* Check illegal parameters */
    if (action == NULL || ace_action == NULL) {
        T_E("Input parameter is NULL");
        return rc;
    }

    // ipolicer
    action->policer = ace_action->rate_limiter_id == VTSS_APPL_ACL_POLICER_DISABLED ? ACL_MGMT_RATE_LIMITER_NONE : upolicer2ipolicer(ace_action->rate_limiter_id);

    // Mirror
    action->mirror = ace_action->mirror;

    // Logging
    action->logging = ace_action->logging;

    // Shutdown
    action->shutdown = ace_action->shutdown;

    T_D("exit");

    return VTSS_RC_OK;
}

/* Check ACL ACE application configuration parameters */
static BOOL _ace_params_invalid(
    IN const vtss_appl_acl_config_ace_t     *const appl_ace,
    IN mesa_ace_type_t                      orig_frame_type
)
{
    T_D("enter");
    if (_ace_action_params_invalid(&appl_ace->action)) {
        T_D("exit: Illegal parameters - Action");
        return TRUE;
    }

    // Hit action
    switch (appl_ace->action.hit_action) {
    case VTSS_APPL_ACL_HIT_ACTION_PERMIT:
    case VTSS_APPL_ACL_HIT_ACTION_DENY:
    case VTSS_APPL_ACL_HIT_ACTION_REDIRECT:
        break;

    case VTSS_APPL_ACL_HIT_ACTION_EGRESS:
        break;

    default:
        return TRUE;
    }

    /* Ingress type checking */
    if (appl_ace->key.ingress_mode == VTSS_APPL_ACL_ACE_INGRESS_MODE_SWITCH ||
        appl_ace->key.ingress_mode == VTSS_APPL_ACL_ACE_INGRESS_MODE_SWITCHPORT) {
        T_D("ingress type is not supportedy");
        return VTSS_RC_ERROR;
    }

    // Policy/bitmask
    if (appl_ace->key.policy.value > ACL_MGMT_POLICY_NO_MAX) {
        T_D("exit: Illegal parameters - policy value(%u)", appl_ace->key.policy.value);
        return TRUE;
    }
    if (appl_ace->key.policy.mask > ACL_MGMT_POLICIES_BITMASK) {
        T_D("exit: Illegal parameters - policy mask(0x%x)", appl_ace->key.policy.mask);
        return TRUE;
    }

    // VLAN ID
    if (appl_ace->key.vlan.vid > VTSS_APPL_VLAN_ID_MAX) {
        T_D("exit: Illegal parameters - vid(%d)", appl_ace->key.vlan.vid);
        return TRUE;
    }

    // VLAN user priority
    if (appl_ace->key.vlan.usr_prio >= VTSS_APPL_VCAP_VLAN_PRI_TYPE_CNT) {
        T_D("exit: Illegal parameters - VLAN user priority(%d)", appl_ace->key.vlan.usr_prio);
        return TRUE;
    }

    // VLAN tagged
    if (appl_ace->key.vlan.tagged >= VTSS_APPL_ACL_ACE_VLAN_TAGGED_CNT) {
        T_D("exit: Illegal parameters - VLAN tagged(%d)", appl_ace->key.vlan.tagged);
        return TRUE;
    }
    if (appl_ace->key.vlan.tagged == VTSS_APPL_ACL_ACE_VLAN_UNTAGGED &&
        appl_ace->key.vlan.usr_prio != VTSS_APPL_VCAP_VLAN_PRI_TYPE_ANY) {
        T_D("exit: Illegal parameters - VLAN user priority cannot set when VLAN taggged filer is disabled");
        return TRUE;
    }

#if 0 // Allan: just ignore not supported items
    // Reject wrong frame type parameters
    memset(&empty_frame, 0, sizeof(empty_frame));
    etype_conf_change = (orig_frame_type == MESA_ACE_TYPE_ETYPE) ? FALSE : memcmp(&empty_frame.ether, &appl_ace->key.frame.ether, sizeof(appl_ace->key.frame.ether));
    arp_conf_change   = (orig_frame_type == MESA_ACE_TYPE_ARP)   ? FALSE : memcmp(&empty_frame.arp,   &appl_ace->key.frame.arp,   sizeof(appl_ace->key.frame.arp));
    ipv4_conf_change  = (orig_frame_type == MESA_ACE_TYPE_IPV4)  ? FALSE : memcmp(&empty_frame.ipv4,  &appl_ace->key.frame.ipv4,  sizeof(appl_ace->key.frame.ipv4));
    ipv6_conf_change  = (orig_frame_type == MESA_ACE_TYPE_IPV6)  ? FALSE : memcmp(&empty_frame.ipv6,  &appl_ace->key.frame.ipv6,  sizeof(appl_ace->key.frame.ipv6));
#endif

    switch (appl_ace->key.frame.frame_type) {
    case VTSS_APPL_ACL_ACE_FRAME_TYPE_ANY:
#if 0 // Allan: just ignore not supported items
        // DMAC type
        if (appl_ace->key.frame.dmac_type != VTSS_APPL_VCAP_ADV_DMAC_TYPE_ANY) {
            T_D("exit: Illegal parameters - DMAC type(%d)", appl_ace->key.frame.dmac_type);
            return TRUE;
        }
#endif
        break;

    case VTSS_APPL_ACL_ACE_FRAME_TYPE_LLC:
    case VTSS_APPL_ACL_ACE_FRAME_TYPE_SNAP:
        T_D("exit: Illegal parameters - frame type(%d)", appl_ace->key.frame.frame_type);
        return TRUE;

    case VTSS_APPL_ACL_ACE_FRAME_TYPE_ETYPE:
#if 0 // Allan: just ignore not supported items
        if (arp_conf_change) {
            T_D("arp_conf_change");
            return TRUE;
        }

        if (ipv4_conf_change) {
            T_D("ipv4_conf_change");
            return TRUE;
        }

        if (ipv6_conf_change) {
            T_D("ipv6_conf_change");
            return TRUE;
        }
#endif

        // DMAC type
        if (appl_ace->key.frame.dmac_type >= VTSS_APPL_VCAP_ADV_DMAC_TYPE_CNT) {
            T_D("exit: Illegal parameters - Etype DMAC type %u", appl_ace->key.frame.dmac_type);
            return TRUE;
        }

        // Etype SMAC type
        if (appl_ace->key.frame.ether.smac_type >= VTSS_APPL_VCAP_AS_TYPE_CNT) {
            T_D("exit: Illegal parameters - Etype SMAC type %u", appl_ace->key.frame.ether.smac_type);
            return TRUE;
        }

        // Etype value
        if (appl_ace->key.frame.ether.etype &&
            (appl_ace->key.frame.ether.etype < 0x600 ||
             appl_ace->key.frame.ether.etype == 0x0800 ||
             appl_ace->key.frame.ether.etype == 0x0806 ||
             appl_ace->key.frame.ether.etype == 0x86DD)) {
            T_D("exit: Illegal parameters - Etype value(0x%x)", appl_ace->key.frame.ether.etype);
            return TRUE;
        }
        break;

    case VTSS_APPL_ACL_ACE_FRAME_TYPE_ARP:
#if 0 // Allan: just ignore not supported items
        if (etype_conf_change) {
            T_D("etype_conf_change");
            return TRUE;
        }

        if (ipv4_conf_change) {
            T_D("ipv4_conf_change");
            return TRUE;
        }

        if (ipv6_conf_change) {
            T_D("ipv6_conf_change");
            return TRUE;
        }
#endif

        // DMAC type
        if (appl_ace->key.frame.dmac_type >= VTSS_APPL_VCAP_ADV_DMAC_TYPE_SPECIFIC) {
            T_D("exit: Illegal parameters - ARP DMAC type %u", appl_ace->key.frame.dmac_type);
            return TRUE;
        }

        // ARP SMAC type
        if (appl_ace->key.frame.arp.smac_type >= VTSS_APPL_VCAP_AS_TYPE_CNT) {
            T_D("exit: Illegal parameters - Etype SMAC type %u", appl_ace->key.frame.arp.smac_type);
            return TRUE;
        }

        // ARP opcode
        if (appl_ace->key.frame.arp.flag.opcode >= VTSS_APPL_ACL_ACE_ARP_OP_CNT) {
            T_D("exit: Illegal parameters - ARP opcode(%d)", appl_ace->key.frame.arp.flag.opcode);
            return TRUE;
        }

        // ARP flags
        if (appl_ace->key.frame.arp.flag.req > MESA_VCAP_BIT_1 ||
            appl_ace->key.frame.arp.flag.sha > MESA_VCAP_BIT_1 ||
            appl_ace->key.frame.arp.flag.tha > MESA_VCAP_BIT_1 ||
            appl_ace->key.frame.arp.flag.hln > MESA_VCAP_BIT_1 ||
            appl_ace->key.frame.arp.flag.hrd > MESA_VCAP_BIT_1 ||
            appl_ace->key.frame.arp.flag.pro > MESA_VCAP_BIT_1) {
            T_D("exit: Illegal parameters - ARP flags");
            return TRUE;
        }
        break;

    case VTSS_APPL_ACL_ACE_FRAME_TYPE_IPV4:
#if 0 // Allan: just ignore not supported items
        if (etype_conf_change) {
            T_W("etype_conf_change");
            return TRUE;
        }

        if (arp_conf_change) {
            T_W("arp_conf_change");
            return TRUE;
        }

        if (ipv6_conf_change) {
            T_W("ipv6_conf_change");
            return TRUE;
        }
#endif

        // DMAC type
        if (appl_ace->key.frame.dmac_type >= VTSS_APPL_VCAP_ADV_DMAC_TYPE_SPECIFIC) {
            T_D("exit: Illegal parameters - IPv4 DMAC type %u", appl_ace->key.frame.dmac_type);
            return TRUE;
        }

        // IPv4 protocol
        if (appl_ace->key.frame.ipv4.proto_type > VTSS_APPL_VCAP_AS_TYPE_CNT ||
            appl_ace->key.frame.ipv4.proto > 255) {
            T_W("exit: Illegal parameters - IPv4 protocol type(%d)", appl_ace->key.frame.ipv4.proto_type);
            return TRUE;
        }

        // IPv4 flags
        if (appl_ace->key.frame.ipv4.ipv4_flag.ttl > MESA_VCAP_BIT_1 ||
            appl_ace->key.frame.ipv4.ipv4_flag.fragment > MESA_VCAP_BIT_1 ||
            appl_ace->key.frame.ipv4.ipv4_flag.option > MESA_VCAP_BIT_1) {
            T_W("exit: Illegal parameters - IPv4 flags");
            return TRUE;
        }

        // IPv4 ICMP type/code
        if (appl_ace->key.frame.ipv4.icmp.type_match > VTSS_APPL_VCAP_AS_TYPE_CNT ||
            appl_ace->key.frame.ipv4.icmp.type > 255 ||
            appl_ace->key.frame.ipv4.icmp.code_match > VTSS_APPL_VCAP_AS_TYPE_CNT ||
            appl_ace->key.frame.ipv4.icmp.code > 255) {
            T_W("exit: Illegal parameters - IPv4 ICMP type/code");
            return TRUE;
        }

        // IPv4 TCP/UDP port
        if (appl_ace->key.frame.ipv4.sport.match > VTSS_APPL_VCAP_ASR_TYPE_CNT ||
            appl_ace->key.frame.ipv4.sport.low > 65535 ||
            appl_ace->key.frame.ipv4.sport.high > 65535 ||
            (appl_ace->key.frame.ipv4.sport.match == VTSS_APPL_VCAP_ASR_TYPE_RANGE && (appl_ace->key.frame.ipv4.sport.low > appl_ace->key.frame.ipv4.sport.high)) ||
            appl_ace->key.frame.ipv4.dport.match > VTSS_APPL_VCAP_ASR_TYPE_CNT ||
            appl_ace->key.frame.ipv4.dport.low > 65535 ||
            appl_ace->key.frame.ipv4.dport.high > 65535 ||
            (appl_ace->key.frame.ipv4.dport.match == VTSS_APPL_VCAP_ASR_TYPE_RANGE && (appl_ace->key.frame.ipv4.dport.low > appl_ace->key.frame.ipv4.dport.high))) {
            T_W("exit: Illegal parameters - IPv4 TCP/UDP port");
            return TRUE;
        }

        // IPv4 TCP flags
        if (appl_ace->key.frame.ipv4.tcp_flag.syn > MESA_VCAP_BIT_1 ||
            appl_ace->key.frame.ipv4.tcp_flag.fin > MESA_VCAP_BIT_1 ||
            appl_ace->key.frame.ipv4.tcp_flag.rst > MESA_VCAP_BIT_1 ||
            appl_ace->key.frame.ipv4.tcp_flag.psh > MESA_VCAP_BIT_1 ||
            appl_ace->key.frame.ipv4.tcp_flag.ack > MESA_VCAP_BIT_1 ||
            appl_ace->key.frame.ipv4.tcp_flag.urg > MESA_VCAP_BIT_1) {
            T_W("exit: Illegal parameters - IPv4 TCP flags");
            return TRUE;
        }
        break;

    case VTSS_APPL_ACL_ACE_FRAME_TYPE_IPV6:
#if 0 // Allan: just ignore not supported items
        if (etype_conf_change) {
            T_D("etype_conf_change");
            return TRUE;
        }

        if (arp_conf_change) {
            T_D("arp_conf_change");
            return TRUE;
        }

        if (ipv4_conf_change) {
            T_D("ipv6_conf_change");
            return TRUE;
        }
#endif

        // DMAC type
        if (appl_ace->key.frame.dmac_type >= VTSS_APPL_VCAP_ADV_DMAC_TYPE_SPECIFIC) {
            T_D("exit: Illegal parameters - IPv6 DMAC type %u", appl_ace->key.frame.dmac_type);
            return TRUE;
        }

        // IPv6 next header type
        if (appl_ace->key.frame.ipv6.next_header_type > VTSS_APPL_VCAP_AS_TYPE_CNT ||
            appl_ace->key.frame.ipv6.next_header > 255) {
            T_D("exit: Illegal parameters - IPv6 next header type(%d)", appl_ace->key.frame.ipv6.next_header_type);
            return TRUE;
        }

        // IPv6 flags
        if (appl_ace->key.frame.ipv6.ipv6_flag.ttl > MESA_VCAP_BIT_1) {
            T_D("exit: Illegal parameters - IPv6 flags");
            return TRUE;
        }

        // IPv6 ICMP
        if (appl_ace->key.frame.ipv6.icmp.type_match > VTSS_APPL_VCAP_AS_TYPE_CNT ||
            appl_ace->key.frame.ipv6.icmp.type > 255 ||
            appl_ace->key.frame.ipv6.icmp.code_match > VTSS_APPL_VCAP_AS_TYPE_CNT ||
            appl_ace->key.frame.ipv6.icmp.code > 255) {
            T_D("exit: Illegal parameters - IPv6 ICMPv6 type");
            return TRUE;
        }

        // IPv6 TCP/UDP port
        if (appl_ace->key.frame.ipv6.sport.match > VTSS_APPL_VCAP_ASR_TYPE_CNT ||
            appl_ace->key.frame.ipv6.sport.low > 65535 ||
            appl_ace->key.frame.ipv6.sport.high > 65535 ||
            (appl_ace->key.frame.ipv6.sport.match == VTSS_APPL_VCAP_ASR_TYPE_RANGE && (appl_ace->key.frame.ipv6.sport.low > appl_ace->key.frame.ipv6.sport.high)) ||
            appl_ace->key.frame.ipv6.dport.match > VTSS_APPL_VCAP_ASR_TYPE_CNT ||
            appl_ace->key.frame.ipv6.dport.low > 65535 ||
            appl_ace->key.frame.ipv6.dport.high > 65535 ||
            (appl_ace->key.frame.ipv6.dport.match == VTSS_APPL_VCAP_ASR_TYPE_RANGE && (appl_ace->key.frame.ipv6.dport.low > appl_ace->key.frame.ipv6.dport.high))) {
            T_D("exit: Illegal parameters - IPv6 TCP/UDP port");
            return TRUE;
        }

        // IPv6 TCP flags
        if (appl_ace->key.frame.ipv6.tcp_flag.syn > MESA_VCAP_BIT_1 ||
            appl_ace->key.frame.ipv6.tcp_flag.fin > MESA_VCAP_BIT_1 ||
            appl_ace->key.frame.ipv6.tcp_flag.rst > MESA_VCAP_BIT_1 ||
            appl_ace->key.frame.ipv6.tcp_flag.psh > MESA_VCAP_BIT_1 ||
            appl_ace->key.frame.ipv6.tcp_flag.ack > MESA_VCAP_BIT_1 ||
            appl_ace->key.frame.ipv6.tcp_flag.urg > MESA_VCAP_BIT_1) {
            T_D("exit: Illegal parameters - IPv6 TCP flags");
            return TRUE;
        }
        break;

    default:
        break;
    }

    return FALSE;
}

static vtss_appl_acl_ace_ingress_mode_t _acl_ace_ingress_mode_get(acl_entry_conf_t *ace)
{
    vtss_appl_acl_ace_ingress_mode_t ingress_mode = VTSS_APPL_ACL_ACE_INGRESS_MODE_ANY;

    mesa_port_no_t port_idx;
    BOOL is_all_ports = TRUE;
    u32  port_count = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    for (port_idx = 0; port_idx < port_count; port_idx++) {
        if (!ace->port_list[port_idx]) {
            is_all_ports = FALSE;
            break;
        }
    }

    if (ace->isid != VTSS_ISID_GLOBAL || !is_all_ports) {
        ingress_mode = VTSS_APPL_ACL_ACE_INGRESS_MODE_SPECIFIC;
    }

    return ingress_mode;
}

/* Use default ACE rule (permit all) when ace_id = 0 */
static mesa_rc _ace_export(
    IN  mesa_ace_id_t               ace_id,
    OUT vtss_appl_acl_config_ace_t  *appl_ace
)
{
    mesa_rc          rc = VTSS_APPL_ACL_ERROR_PARM;
    acl_entry_conf_t ace;
    mesa_mac_t       empty_mask;
    mesa_port_no_t   iport;
    int              action_port_list_cnt;
    u32              port_count = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    /* Check illegal parameters */
    if (appl_ace == NULL) {
        T_E("Input parameter is NULL");
        return rc;
    }

    if (ace_id) {
        if (ace_id < ACL_MGMT_ACE_ID_START || ace_id > ACL_MGMT_ACE_ID_END) {
            T_E("exit: Illegal parameters");
            return VTSS_APPL_ACL_ERROR_PARM;
        }

        /* Clear application configuration database */
        memset(appl_ace, 0, sizeof(*appl_ace));

        // Next ACE ID
        if (acl_mgmt_ace_get(ACL_USER_STATIC, ace_id, &ace, NULL, TRUE) == VTSS_RC_OK) {
            appl_ace->next_ace_id = ace.id;
        } else {
            appl_ace->next_ace_id = ACL_MGMT_ACE_ID_NONE;
        }

        /* Get ACE configuration */
        if ((rc = acl_mgmt_ace_get(ACL_USER_STATIC, ace_id, &ace, NULL, FALSE)) != VTSS_RC_OK) {
            T_D("exit: calling acl_mgmt_ace_get(ace_id=%u) failed", ace_id);
            return rc;
        }
    } else if ((rc = acl_mgmt_ace_init(MESA_ACE_TYPE_ANY, &ace)) != VTSS_RC_OK) {
        return rc;
    }

    // ACE action
    rc = _ace_action_export(&ace.action, &appl_ace->action);
    if (rc != VTSS_RC_OK) {
        T_D("exit: fail to export ace action");
        return rc;
    }

    /* Export ACE application configuration */
    // Lookup
    appl_ace->key.second_lookup = ace.lookup;

    // Policy
    appl_ace->key.policy = ace.policy;

    appl_ace->key.ingress_mode = _acl_ace_ingress_mode_get(&ace);

    // Ingress port list
    if (appl_ace->key.ingress_mode == VTSS_APPL_ACL_ACE_INGRESS_MODE_SPECIFIC &&
        _stackable_set_by_port_list(ace.isid, ace.port_list, &(appl_ace->key.ingress_port)) == FALSE) {
        T_W("_stackable_set_by_port_list(%u, ingress_port)", ace.isid);
        return VTSS_RC_ERROR;
    }

    // get port count
    action_port_list_cnt = 0;
    for (iport = 0; iport < port_count; iport++) {
        if (ace.action.port_list[iport]) {
            action_port_list_cnt++;
        }
    }

    switch (ace.action.port_action) {
    case MESA_ACL_PORT_ACTION_NONE:
        appl_ace->action.hit_action = VTSS_APPL_ACL_HIT_ACTION_PERMIT;
        break;

    case MESA_ACL_PORT_ACTION_FILTER:
        if (action_port_list_cnt) {
            if (_stackable_set_by_port_list(ace.isid, ace.action.port_list, &(appl_ace->action.egress_port)) == FALSE) {
                T_W("_stackable_set_by_port_list(%u, egress_port)", ace.isid);
                return VTSS_RC_ERROR;
            }
            appl_ace->action.hit_action = VTSS_APPL_ACL_HIT_ACTION_EGRESS;
        } else {
            appl_ace->action.hit_action = VTSS_APPL_ACL_HIT_ACTION_DENY;
        }
        break;

    case MESA_ACL_PORT_ACTION_REDIR:
        if (action_port_list_cnt) {
            if (_stackable_set_by_port_list(ace.isid, ace.action.port_list, &(appl_ace->action.redirect_port)) == FALSE) {
                T_W("_stackable_set_by_port_list(%u, redirect_port)", ace.isid);
                return VTSS_RC_ERROR;
            }
            appl_ace->action.hit_action = VTSS_APPL_ACL_HIT_ACTION_REDIRECT;
        } else {
            appl_ace->action.hit_action = VTSS_APPL_ACL_HIT_ACTION_DENY;
        }
        break;

    default:
        T_W("invalid port action %u", ace.action.port_action);
        return VTSS_RC_ERROR;
    }

    // VLAN ID
    appl_ace->key.vlan.vid = vtss_appl_vcap_vid_type2value(ace.vid);

    // VLAN user priority
    appl_ace->key.vlan.usr_prio = vtss_appl_vcap_vlan_pri2pri_type(ace.usr_prio);

    // VLAN tagged
    appl_ace->key.vlan.tagged = _vcap_tagged_type2value(ace.tagged);

    // Frame type
    appl_ace->key.frame.frame_type = (vtss_appl_acl_ace_frame_type_t) ace.type;

    memset(&empty_mask, 0, sizeof(empty_mask));
    switch (appl_ace->key.frame.frame_type) {
    case VTSS_APPL_ACL_ACE_FRAME_TYPE_ANY:
        // Any frame DMAC
        break;

    case VTSS_APPL_ACL_ACE_FRAME_TYPE_ETYPE:
        // smac
        appl_ace->key.frame.ether.smac_type = memcmp(ace.frame.etype.smac.mask, empty_mask.addr, 6) ? VTSS_APPL_VCAP_AS_TYPE_SPECIFIC : VTSS_APPL_VCAP_AS_TYPE_ANY;
        if (appl_ace->key.frame.ether.smac_type == VTSS_APPL_VCAP_AS_TYPE_SPECIFIC) {
            memcpy(appl_ace->key.frame.ether.smac.addr, ace.frame.etype.smac.value, 6);
        }

        // dmac
        if (memcmp(ace.frame.etype.dmac.mask, empty_mask.addr, 6)) {
            appl_ace->key.frame.dmac_type = VTSS_APPL_VCAP_ADV_DMAC_TYPE_SPECIFIC;
            memcpy(appl_ace->key.frame.ether.dmac.addr, ace.frame.etype.dmac.value, 6);
        } else {
            acl_flag_t ace_bc_flag, ace_mc_flag;
            ace_bc_flag = VTSS_BF_GET(ace.flags.mask, ACE_FLAG_DMAC_BC) ? (VTSS_BF_GET(ace.flags.value, ACE_FLAG_DMAC_BC) ? 1 : 0) : 2;
            ace_mc_flag = VTSS_BF_GET(ace.flags.mask, ACE_FLAG_DMAC_MC) ? (VTSS_BF_GET(ace.flags.value, ACE_FLAG_DMAC_MC) ? 1 : 0) : 2;
            if (ace_bc_flag == 2 && ace_mc_flag == 2) { // Any
                appl_ace->key.frame.dmac_type = VTSS_APPL_VCAP_ADV_DMAC_TYPE_ANY;
            } else if (ace_bc_flag == 0 && ace_mc_flag == 1) { // MC
                appl_ace->key.frame.dmac_type = VTSS_APPL_VCAP_ADV_DMAC_TYPE_MULTICAST;
            } else if (ace_bc_flag == 1 && ace_mc_flag == 1) { // BC
                appl_ace->key.frame.dmac_type = VTSS_APPL_VCAP_ADV_DMAC_TYPE_BROADCAST;
            } else if (ace_bc_flag == 0 && ace_mc_flag == 0) { // UC
                appl_ace->key.frame.dmac_type = VTSS_APPL_VCAP_ADV_DMAC_TYPE_UNICAST;
            }
        }

        // ether
        appl_ace->key.frame.ether.etype = vtss_appl_vcap_etype_type2value(ace.frame.etype.etype);
        break;

    case VTSS_APPL_ACL_ACE_FRAME_TYPE_ARP:
        // ARP SMAC
        appl_ace->key.frame.arp.smac_type = memcmp(ace.frame.arp.smac.mask, empty_mask.addr, 6) ? VTSS_APPL_VCAP_AS_TYPE_SPECIFIC : VTSS_APPL_VCAP_AS_TYPE_ANY;
        if (appl_ace->key.frame.arp.smac_type == VTSS_APPL_VCAP_AS_TYPE_SPECIFIC) {
            memcpy(appl_ace->key.frame.arp.smac.addr, ace.frame.arp.smac.value, 6);
        }

        // ARP DMAC
        appl_ace->key.frame.dmac_type = _dmac_export(VTSS_APPL_ACL_ACE_FRAME_TYPE_ARP, &ace);

        // SRP SIP/DIP
        appl_ace->key.frame.arp.sip = ace.frame.arp.sip;
        appl_ace->key.frame.arp.dip = ace.frame.arp.dip;

        // ARP opcode
        appl_ace->key.frame.arp.flag.opcode = VTSS_BF_GET(ace.flags.mask, ACE_FLAG_ARP_ARP) ? (VTSS_BF_GET(ace.flags.value, ACE_FLAG_ARP_ARP) ? VTSS_APPL_ACL_ACE_ARP_OP_ARP : VTSS_APPL_ACL_ACE_ARP_OP_RARP) : VTSS_BF_GET(ace.flags.mask, ACE_FLAG_ARP_UNKNOWN) ? VTSS_APPL_ACL_ACE_ARP_OP_OTHER : VTSS_APPL_ACL_ACE_ARP_OP_ANY;

        // ARP flags
        appl_ace->key.frame.arp.flag.req = _flag_export(ACE_FLAG_ARP_REQ, &ace);
        appl_ace->key.frame.arp.flag.sha = _flag_export(ACE_FLAG_ARP_SMAC, &ace);
        appl_ace->key.frame.arp.flag.tha = _flag_export(ACE_FLAG_ARP_DMAC, &ace);
        appl_ace->key.frame.arp.flag.hln = _flag_export(ACE_FLAG_ARP_LEN, &ace);
        appl_ace->key.frame.arp.flag.hrd = _flag_export(ACE_FLAG_ARP_IP, &ace);
        appl_ace->key.frame.arp.flag.pro = _flag_export(ACE_FLAG_ARP_ETHER, &ace);
        break;

    case VTSS_APPL_ACL_ACE_FRAME_TYPE_IPV4:
        // IPv4 DMAC
        appl_ace->key.frame.dmac_type = _dmac_export(VTSS_APPL_ACL_ACE_FRAME_TYPE_IPV4, &ace);

        // IPv4 protocol
        appl_ace->key.frame.ipv4.proto_type = ace.frame.ipv4.proto.mask ? VTSS_APPL_VCAP_AS_TYPE_SPECIFIC : VTSS_APPL_VCAP_AS_TYPE_ANY;
        appl_ace->key.frame.ipv4.proto = ace.frame.ipv4.proto.mask ? ace.frame.ipv4.proto.value : 0;

        // IPV4 flags
        appl_ace->key.frame.ipv4.ipv4_flag.ttl      = _flag_export(ACE_FLAG_IP_TTL, &ace);
        appl_ace->key.frame.ipv4.ipv4_flag.fragment = _flag_export(ACE_FLAG_IP_FRAGMENT, &ace);
        appl_ace->key.frame.ipv4.ipv4_flag.option   = _flag_export(ACE_FLAG_IP_OPTIONS, &ace);

        // IPv4 SIP/DIP
        appl_ace->key.frame.ipv4.sip = ace.frame.ipv4.sip;
        appl_ace->key.frame.ipv4.dip = ace.frame.ipv4.dip;

        // IPv4 ICMP
        if (ace.frame.ipv4.proto.mask && ace.frame.ipv4.proto.value == 1) {
            appl_ace->key.frame.ipv4.icmp.type_match = ace.frame.ipv4.data.mask[0] ? VTSS_APPL_VCAP_AS_TYPE_SPECIFIC : VTSS_APPL_VCAP_AS_TYPE_ANY;
            appl_ace->key.frame.ipv4.icmp.type = ace.frame.ipv4.data.mask[0] ? ace.frame.ipv4.data.value[0] : 0;
            appl_ace->key.frame.ipv4.icmp.code_match = ace.frame.ipv4.data.mask[1] ? VTSS_APPL_VCAP_AS_TYPE_SPECIFIC : VTSS_APPL_VCAP_AS_TYPE_ANY;
            appl_ace->key.frame.ipv4.icmp.code = ace.frame.ipv4.data.mask[1] ? ace.frame.ipv4.data.value[1] : 0;
        }

        // IPv4 TCP/UDP port
        if (ace.frame.ipv4.proto.mask && (ace.frame.ipv4.proto.value == 6 || ace.frame.ipv4.proto.value == 17)) {
            appl_ace->key.frame.ipv4.sport.match = (ace.frame.ipv4.sport.low == 0 && ace.frame.ipv4.sport.high == 0xFFFF) ? VTSS_APPL_VCAP_ASR_TYPE_ANY : (ace.frame.ipv4.sport.low == ace.frame.ipv4.sport.high) ? VTSS_APPL_VCAP_ASR_TYPE_SPECIFIC : VTSS_APPL_VCAP_ASR_TYPE_RANGE;
            appl_ace->key.frame.ipv4.sport.low = (appl_ace->key.frame.ipv4.sport.match != VTSS_APPL_VCAP_ASR_TYPE_ANY) ? ace.frame.ipv4.sport.low : 0;
            appl_ace->key.frame.ipv4.sport.high = (appl_ace->key.frame.ipv4.sport.match == VTSS_APPL_VCAP_ASR_TYPE_RANGE) ? ace.frame.ipv4.sport.high : 0;
            appl_ace->key.frame.ipv4.dport.match = (ace.frame.ipv4.dport.low == 0 && ace.frame.ipv4.dport.high == 0xFFFF) ? VTSS_APPL_VCAP_ASR_TYPE_ANY : (ace.frame.ipv4.dport.low == ace.frame.ipv4.dport.high) ? VTSS_APPL_VCAP_ASR_TYPE_SPECIFIC : VTSS_APPL_VCAP_ASR_TYPE_RANGE;
            appl_ace->key.frame.ipv4.dport.low = (appl_ace->key.frame.ipv4.dport.match != VTSS_APPL_VCAP_ASR_TYPE_ANY) ? ace.frame.ipv4.dport.low : 0;
            appl_ace->key.frame.ipv4.dport.high = (appl_ace->key.frame.ipv4.dport.match == VTSS_APPL_VCAP_ASR_TYPE_RANGE) ? ace.frame.ipv4.dport.high : 0;

            // IPv4 TCP flags
            if (ace.frame.ipv4.proto.value == 6) {
                appl_ace->key.frame.ipv4.tcp_flag.fin = _flag_export(ACE_FLAG_TCP_FIN, &ace);
                appl_ace->key.frame.ipv4.tcp_flag.syn = _flag_export(ACE_FLAG_TCP_SYN, &ace);
                appl_ace->key.frame.ipv4.tcp_flag.rst = _flag_export(ACE_FLAG_TCP_RST, &ace);
                appl_ace->key.frame.ipv4.tcp_flag.psh = _flag_export(ACE_FLAG_TCP_PSH, &ace);
                appl_ace->key.frame.ipv4.tcp_flag.ack = _flag_export(ACE_FLAG_TCP_ACK, &ace);
                appl_ace->key.frame.ipv4.tcp_flag.urg = _flag_export(ACE_FLAG_TCP_URG, &ace);
            }
        }
        break;

    case VTSS_APPL_ACL_ACE_FRAME_TYPE_IPV6:
        // IPv6 DMAC
        appl_ace->key.frame.dmac_type = _dmac_export(VTSS_APPL_ACL_ACE_FRAME_TYPE_IPV6, &ace);

        // IPv6 next header
        appl_ace->key.frame.ipv6.next_header_type = ace.frame.ipv6.proto.mask ? VTSS_APPL_VCAP_AS_TYPE_SPECIFIC : VTSS_APPL_VCAP_AS_TYPE_ANY;
        appl_ace->key.frame.ipv6.next_header = ace.frame.ipv6.proto.mask ? ace.frame.ipv6.proto.value : 0;

        // IPv6 flags
        appl_ace->key.frame.ipv6.ipv6_flag.ttl = (mesa_vcap_bit_t) ace.frame.ipv6.ttl;

        // IPv6 SIP
        memcpy(&appl_ace->key.frame.ipv6.sip, &ace.frame.ipv6.sip, sizeof(ace.frame.ipv6.sip));

        // IPv6 ICMP
        if (ace.frame.ipv6.proto.mask && ace.frame.ipv6.proto.value == 58) {
            appl_ace->key.frame.ipv6.icmp.type_match = ace.frame.ipv6.data.mask[0] ? VTSS_APPL_VCAP_AS_TYPE_SPECIFIC : VTSS_APPL_VCAP_AS_TYPE_ANY;
            appl_ace->key.frame.ipv6.icmp.type = ace.frame.ipv6.data.mask[0] ? ace.frame.ipv6.data.value[0] : 0;
            appl_ace->key.frame.ipv6.icmp.code_match = ace.frame.ipv6.data.mask[1] ? VTSS_APPL_VCAP_AS_TYPE_SPECIFIC : VTSS_APPL_VCAP_AS_TYPE_ANY;
            appl_ace->key.frame.ipv6.icmp.code = ace.frame.ipv6.data.mask[1] ? ace.frame.ipv6.data.value[1] : 0;
        }

        // IPv6 TCP/UDP port
        if (ace.frame.ipv6.proto.mask && (ace.frame.ipv6.proto.value == 6 || ace.frame.ipv6.proto.value == 17)) {
            appl_ace->key.frame.ipv6.sport.match = (ace.frame.ipv6.sport.low == 0 && ace.frame.ipv6.sport.high == 0xFFFF) ? VTSS_APPL_VCAP_ASR_TYPE_ANY : (ace.frame.ipv6.sport.low == ace.frame.ipv6.sport.high) ? VTSS_APPL_VCAP_ASR_TYPE_SPECIFIC : VTSS_APPL_VCAP_ASR_TYPE_RANGE;
            appl_ace->key.frame.ipv6.sport.low = (appl_ace->key.frame.ipv6.sport.match != VTSS_APPL_VCAP_ASR_TYPE_ANY) ? ace.frame.ipv6.sport.low : 0;
            appl_ace->key.frame.ipv6.sport.high = (appl_ace->key.frame.ipv6.sport.match == VTSS_APPL_VCAP_ASR_TYPE_RANGE) ? ace.frame.ipv6.sport.high : 0;
            appl_ace->key.frame.ipv6.dport.match = (ace.frame.ipv6.dport.low == 0 && ace.frame.ipv6.dport.high == 0xFFFF) ? VTSS_APPL_VCAP_ASR_TYPE_ANY : (ace.frame.ipv6.dport.low == ace.frame.ipv6.dport.high) ? VTSS_APPL_VCAP_ASR_TYPE_SPECIFIC : VTSS_APPL_VCAP_ASR_TYPE_RANGE;
            appl_ace->key.frame.ipv6.dport.low = (appl_ace->key.frame.ipv6.dport.match != VTSS_APPL_VCAP_ASR_TYPE_ANY) ? ace.frame.ipv6.dport.low : 0;
            appl_ace->key.frame.ipv6.dport.high = (appl_ace->key.frame.ipv6.dport.match == VTSS_APPL_VCAP_ASR_TYPE_RANGE) ? ace.frame.ipv6.dport.high : 0;

            // IPv6 TCP flags
            if (ace.frame.ipv6.proto.value == 6) {
                appl_ace->key.frame.ipv6.tcp_flag.fin = (mesa_vcap_bit_t) ace.frame.ipv6.tcp_fin;
                appl_ace->key.frame.ipv6.tcp_flag.syn = (mesa_vcap_bit_t) ace.frame.ipv6.tcp_syn;
                appl_ace->key.frame.ipv6.tcp_flag.rst = (mesa_vcap_bit_t) ace.frame.ipv6.tcp_rst;
                appl_ace->key.frame.ipv6.tcp_flag.psh = (mesa_vcap_bit_t) ace.frame.ipv6.tcp_psh;
                appl_ace->key.frame.ipv6.tcp_flag.ack = (mesa_vcap_bit_t) ace.frame.ipv6.tcp_ack;
                appl_ace->key.frame.ipv6.tcp_flag.urg = (mesa_vcap_bit_t) ace.frame.ipv6.tcp_urg;
            }
        }
        break;

    default:
        T_D("invalid frame type %u", appl_ace->key.frame.frame_type);
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

/* Import ACL ACE application configuration */
static mesa_rc _ace_import(
    IN mesa_ace_id_t                        ace_id,
    IN const vtss_appl_acl_config_ace_t     *const appl_ace,
    IN BOOL                                 is_create
)
{
    mesa_rc             rc = VTSS_APPL_ACL_ERROR_PARM;
    acl_entry_conf_t    ace;
    vtss_isid_t         isid;
    u32                 port_count = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    T_D("enter: ace_id = %u, is_create = %d", ace_id, is_create);

    /* Check illegal parameters */
    if (appl_ace == NULL) {
        T_E("Input parameter is NULL");
        return rc;
    }

    if (ace_id < ACL_MGMT_ACE_ID_START || ace_id > ACL_MGMT_ACE_ID_END) {
        T_D("exit: Illegal parameters");
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    /* Get ACE configuration */
    rc = acl_mgmt_ace_get(ACL_USER_STATIC, ace_id, &ace, NULL, FALSE);
    if (is_create) {
        if (rc == VTSS_RC_OK) {
            T_D("exit: ACE already existing");
            return VTSS_APPL_ACL_ERROR_ACE_ALREDY_EXIST;
        }

        /* Initializate default ACE configuration */
        if ((rc = acl_mgmt_ace_init((mesa_ace_type_t) appl_ace->key.frame.frame_type, &ace)) != VTSS_RC_OK) {
            T_D("exit: calling acl_mgmt_ace_init() failed");
            return rc;
        }
    } else if (rc) {
        T_D("exit: calling acl_mgmt_ace_get() failed");
        return rc;
    }

    if (_ace_params_invalid(appl_ace, ace.type)) {
        T_D("exit: Illegal parameters");
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    // ACE action
    if ((rc = _ace_action_import(&(appl_ace->action), &(ace.action))) != VTSS_RC_OK) {
        T_D("exit: fail to import ace action");
        return rc;
    }

    // Lookup
    ace.lookup = appl_ace->key.second_lookup;

    // Policy
    ace.policy = appl_ace->key.policy;

    /*
        NOTE:
            ace.isid is for ingress port only.
            and ingress port must be configured.
            redirect and filter port work on local switch only.
    */

    // Ingress port list
    if (appl_ace->key.ingress_mode == VTSS_APPL_ACL_ACE_INGRESS_MODE_ANY) {
        isid = VTSS_ISID_GLOBAL;
        mesa_port_no_t port_idx;
        for (port_idx = 0; port_idx < port_count; port_idx++) {
            ace.port_list[port_idx] = TRUE;
        }
    } else if (_no_port_configured(&(appl_ace->key.ingress_port)) ||
               _port_list_get_by_stackable(&(appl_ace->key.ingress_port), &isid, ace.port_list) == FALSE) {
        T_W("_port_list_get_by_stackable(ingress_port)");
        return VTSS_RC_ERROR;
    }

    if (isid == 0) {
        T_D("ingress port list is empty");
        return VTSS_RC_ERROR;
    }

    // get ACE isid
    ace.isid = VTSS_ISID_GLOBAL;

    // reset all ports
    memset(ace.action.port_list, 0, sizeof(ace.action.port_list));

    // Action
    switch (appl_ace->action.hit_action) {
    case VTSS_APPL_ACL_HIT_ACTION_PERMIT:
        // port action
        ace.action.port_action = MESA_ACL_PORT_ACTION_NONE;
        break;

    case VTSS_APPL_ACL_HIT_ACTION_DENY:
        // port action
        ace.action.port_action = MESA_ACL_PORT_ACTION_FILTER;
        break;

    case VTSS_APPL_ACL_HIT_ACTION_REDIRECT:
        if (_port_list_get_by_stackable(&(appl_ace->action.redirect_port), &isid, ace.action.port_list) == FALSE) {
            T_D("_port_list_get_by_stackable(redirect_port)");
            return VTSS_RC_ERROR;
        }

        if (isid == 0) {
            T_D("redirect port list is empty");
            return VTSS_RC_ERROR;
        }

        if (msg_switch_is_local(isid) == FALSE) {
            T_D("redirect_isid is not local isid");
            return VTSS_RC_ERROR;
        }

        // port action
        ace.action.port_action = MESA_ACL_PORT_ACTION_REDIR;
        break;

    case VTSS_APPL_ACL_HIT_ACTION_EGRESS:
        // get filter port list
        if (_port_list_get_by_stackable(&(appl_ace->action.egress_port), &isid, ace.action.port_list) == FALSE) {
            T_W("_port_list_get_by_stackable(egress_port)");
            return VTSS_RC_ERROR;
        }

        if (isid == 0) {
            T_D("egress port list is empty");
            return VTSS_RC_ERROR;
        }

        if (msg_switch_is_local(isid) == FALSE) {
            T_D("egress_isid is not local isid");
            return VTSS_RC_ERROR;
        }

        // port action
        ace.action.port_action = MESA_ACL_PORT_ACTION_FILTER;
        break;

    default:
        T_W("invalid hit action %u", appl_ace->action.hit_action);
        return VTSS_RC_ERROR;
    }

    // VLAN ID
    if (vtss_appl_vcap_vid_value2type(appl_ace->key.vlan.vid, &ace.vid) != VTSS_RC_OK) {
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    // VLAN user priority
    vtss_appl_vcap_pri_type2vlan_pri(appl_ace->key.vlan.usr_prio, &ace.usr_prio);

    // VLAN tagged
    if (_vcap_tagged_value2type(appl_ace->key.vlan.tagged, &ace.tagged)) {
        T_D("_vcap_tagged_value2type()");
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    switch (appl_ace->key.frame.frame_type) {
    case VTSS_APPL_ACL_ACE_FRAME_TYPE_ANY:
#if 0 // Allan: just ignore not supported items
        if (appl_ace->key.frame.dmac_type != VTSS_APPL_VCAP_ADV_DMAC_TYPE_ANY) {
            T_D("frame any DMAC not supported");
            return VTSS_APPL_ACL_ERROR_PARM;
        }
#endif

        ace.type = MESA_ACE_TYPE_ANY;
        break;

    case VTSS_APPL_ACL_ACE_FRAME_TYPE_ETYPE:
        // Etype SMAC
        if (appl_ace->key.frame.ether.smac_type == VTSS_APPL_VCAP_AS_TYPE_SPECIFIC) {
            memcpy(ace.frame.etype.smac.value, appl_ace->key.frame.ether.smac.addr, 6);
            memset(ace.frame.etype.smac.mask, 0xFF, 6);
        } else { // any
            memset(&ace.frame.etype.smac, 0, sizeof(ace.frame.etype.smac));
        }

        // Etype DMAC
        memset(&ace.frame.etype.dmac, 0, sizeof(ace.frame.etype.dmac));
        if (appl_ace->key.frame.dmac_type == VTSS_APPL_VCAP_ADV_DMAC_TYPE_SPECIFIC) {
            memcpy(ace.frame.etype.dmac.value, appl_ace->key.frame.ether.dmac.addr, 6);
            memset(ace.frame.etype.dmac.mask, 0xFF, 6);
        } else if (appl_ace->key.frame.dmac_type == VTSS_APPL_VCAP_ADV_DMAC_TYPE_MULTICAST) {
            VTSS_BF_SET(ace.flags.mask, ACE_FLAG_DMAC_MC, 1);
            VTSS_BF_SET(ace.flags.value, ACE_FLAG_DMAC_MC, 1);
            VTSS_BF_SET(ace.flags.mask, ACE_FLAG_DMAC_BC, 1);
            VTSS_BF_SET(ace.flags.value, ACE_FLAG_DMAC_BC, 0);
        } else if (appl_ace->key.frame.dmac_type == VTSS_APPL_VCAP_ADV_DMAC_TYPE_BROADCAST) {
            VTSS_BF_SET(ace.flags.mask, ACE_FLAG_DMAC_MC, 1);
            VTSS_BF_SET(ace.flags.value, ACE_FLAG_DMAC_MC, 1);
            VTSS_BF_SET(ace.flags.mask, ACE_FLAG_DMAC_BC, 1);
            VTSS_BF_SET(ace.flags.value, ACE_FLAG_DMAC_BC, 1);
        } else if (appl_ace->key.frame.dmac_type == VTSS_APPL_VCAP_ADV_DMAC_TYPE_UNICAST) {
            VTSS_BF_SET(ace.flags.mask, ACE_FLAG_DMAC_MC, 1);
            VTSS_BF_SET(ace.flags.value, ACE_FLAG_DMAC_MC, 0);
            VTSS_BF_SET(ace.flags.mask, ACE_FLAG_DMAC_BC, 1);
            VTSS_BF_SET(ace.flags.value, ACE_FLAG_DMAC_BC, 0);
        } else { // any
            VTSS_BF_SET(ace.flags.mask, ACE_FLAG_DMAC_MC, 0);
            VTSS_BF_SET(ace.flags.value, ACE_FLAG_DMAC_MC, 0);
            VTSS_BF_SET(ace.flags.mask, ACE_FLAG_DMAC_BC, 0);
            VTSS_BF_SET(ace.flags.value, ACE_FLAG_DMAC_BC, 0);
        }

        // Etype value
        if (vtss_appl_vcap_etype_value2type(appl_ace->key.frame.ether.etype, &ace.frame.etype.etype) != VTSS_RC_OK) {
            T_D("vtss_appl_vcap_etype_value2type()");
            return VTSS_APPL_ACL_ERROR_PARM;
        }

        ace.type = MESA_ACE_TYPE_ETYPE;
        break;

    case VTSS_APPL_ACL_ACE_FRAME_TYPE_ARP:
        // ARP SMAC
        if (appl_ace->key.frame.arp.smac_type == VTSS_APPL_VCAP_AS_TYPE_SPECIFIC) {
            memcpy(ace.frame.arp.smac.value, appl_ace->key.frame.arp.smac.addr, 6);
            memset(ace.frame.arp.smac.mask, 0xFF, 6);
        } else { // any
            memset(&ace.frame.arp.smac, 0, sizeof(ace.frame.arp.smac));
        }

        // ARP DMAC
        if (_dmac_import(appl_ace->key.frame.dmac_type, &ace) != VTSS_RC_OK) {
            T_D("_dmac_import()");
            return VTSS_APPL_ACL_ERROR_PARM;
        }

        // ARP SIP/DIP
        ace.frame.arp.sip = appl_ace->key.frame.arp.sip;
        ace.frame.arp.dip = appl_ace->key.frame.arp.dip;

        // ARP opcode
        if (appl_ace->key.frame.arp.flag.opcode == VTSS_APPL_ACL_ACE_ARP_OP_ARP) {
            VTSS_BF_SET(ace.flags.mask, ACE_FLAG_ARP_ARP, 1);
            VTSS_BF_SET(ace.flags.value, ACE_FLAG_ARP_ARP, 1);
            VTSS_BF_SET(ace.flags.mask, ACE_FLAG_ARP_UNKNOWN, 1);
            VTSS_BF_SET(ace.flags.value, ACE_FLAG_ARP_UNKNOWN, 0);
        } else if (appl_ace->key.frame.arp.flag.opcode == VTSS_APPL_ACL_ACE_ARP_OP_RARP) {
            VTSS_BF_SET(ace.flags.mask, ACE_FLAG_ARP_ARP, 1);
            VTSS_BF_SET(ace.flags.value, ACE_FLAG_ARP_ARP, 0);
            VTSS_BF_SET(ace.flags.mask, ACE_FLAG_ARP_UNKNOWN, 1);
            VTSS_BF_SET(ace.flags.value, ACE_FLAG_ARP_UNKNOWN, 0);
        } else if (appl_ace->key.frame.arp.flag.opcode == VTSS_APPL_ACL_ACE_ARP_OP_OTHER) {
            VTSS_BF_SET(ace.flags.mask, ACE_FLAG_ARP_ARP, 0);
            VTSS_BF_SET(ace.flags.value, ACE_FLAG_ARP_ARP, 0);
            VTSS_BF_SET(ace.flags.mask, ACE_FLAG_ARP_UNKNOWN, 1);
            VTSS_BF_SET(ace.flags.value, ACE_FLAG_ARP_UNKNOWN, 1);
        } else { // any
            VTSS_BF_SET(ace.flags.mask, ACE_FLAG_ARP_ARP, 0);
            VTSS_BF_SET(ace.flags.value, ACE_FLAG_ARP_ARP, 0);
            VTSS_BF_SET(ace.flags.mask, ACE_FLAG_ARP_UNKNOWN, 0);
            VTSS_BF_SET(ace.flags.value, ACE_FLAG_ARP_UNKNOWN, 0);
        }

        // ARP flags
        _flag_import(appl_ace->key.frame.arp.flag.req, ACE_FLAG_ARP_REQ, &ace);
        _flag_import(appl_ace->key.frame.arp.flag.sha, ACE_FLAG_ARP_SMAC, &ace);
        _flag_import(appl_ace->key.frame.arp.flag.tha, ACE_FLAG_ARP_DMAC, &ace);
        _flag_import(appl_ace->key.frame.arp.flag.hln, ACE_FLAG_ARP_LEN, &ace);
        _flag_import(appl_ace->key.frame.arp.flag.hrd, ACE_FLAG_ARP_IP, &ace);
        _flag_import(appl_ace->key.frame.arp.flag.pro, ACE_FLAG_ARP_ETHER, &ace);

        ace.type = MESA_ACE_TYPE_ARP;
        break;

    case VTSS_APPL_ACL_ACE_FRAME_TYPE_IPV4:
        // IPv4 DMAC
        if (_dmac_import(appl_ace->key.frame.dmac_type, &ace) != VTSS_RC_OK) {
            T_D("_dmac_import()");
            return VTSS_APPL_ACL_ERROR_PARM;
        }

        // IPv4 protocol
        if (appl_ace->key.frame.ipv4.proto_type == VTSS_APPL_VCAP_AS_TYPE_SPECIFIC) {
            ace.frame.ipv4.proto.value = appl_ace->key.frame.ipv4.proto;
            ace.frame.ipv4.proto.mask = 0xFF;
        } else { // any
            ace.frame.ipv4.proto.value = ace.frame.ipv4.proto.mask = 0;
        }

        // IPv4 flags
        _flag_import(appl_ace->key.frame.ipv4.ipv4_flag.ttl, ACE_FLAG_IP_TTL, &ace);
        _flag_import(appl_ace->key.frame.ipv4.ipv4_flag.fragment, ACE_FLAG_IP_FRAGMENT, &ace);
        _flag_import(appl_ace->key.frame.ipv4.ipv4_flag.option, ACE_FLAG_IP_OPTIONS, &ace);

        // IPv4 SIP/DIP
        ace.frame.ipv4.sip = appl_ace->key.frame.ipv4.sip;
        ace.frame.ipv4.dip = appl_ace->key.frame.ipv4.dip;

        // IPv4 ICMP
        if (appl_ace->key.frame.ipv4.proto_type == VTSS_APPL_VCAP_AS_TYPE_SPECIFIC &&
            appl_ace->key.frame.ipv4.proto == 1) {
            if (appl_ace->key.frame.ipv4.icmp.type_match == VTSS_APPL_VCAP_AS_TYPE_SPECIFIC) {
                ace.frame.ipv4.data.value[0] = appl_ace->key.frame.ipv4.icmp.type;
                ace.frame.ipv4.data.mask[0] = 0xFF;
            } else { // any
                ace.frame.ipv4.data.value[0] =  ace.frame.ipv4.data.mask[0] = 0;
            }
            if (appl_ace->key.frame.ipv4.icmp.code_match == VTSS_APPL_VCAP_AS_TYPE_SPECIFIC) {
                ace.frame.ipv4.data.value[1] = appl_ace->key.frame.ipv4.icmp.code;
                ace.frame.ipv4.data.mask[1] = 0xFF;
            } else { // any
                ace.frame.ipv4.data.value[1] =  ace.frame.ipv4.data.mask[1] = 0;
            }
        } else {
            memset(&ace.frame.ipv4.data, 0, sizeof(ace.frame.ipv4.data));
        }

        // IPv4 TCP/UDP source port
        ace.frame.ipv4.sport.in_range = 1;
        if (appl_ace->key.frame.ipv4.sport.match == VTSS_APPL_VCAP_ASR_TYPE_SPECIFIC) {
            ace.frame.ipv4.sport.low = ace.frame.ipv4.sport.high = appl_ace->key.frame.ipv4.sport.low;
        } else if (appl_ace->key.frame.ipv4.sport.match == VTSS_APPL_VCAP_ASR_TYPE_RANGE) {
            ace.frame.ipv4.sport.low = appl_ace->key.frame.ipv4.sport.low;
            ace.frame.ipv4.sport.high = appl_ace->key.frame.ipv4.sport.high;
        } else { // any
            ace.frame.ipv4.sport.low = 0;
            ace.frame.ipv4.sport.high = 0xFFFF;
        }

        // IPv4 TCP/UDP destination port
        ace.frame.ipv4.dport.in_range = 1;
        if (appl_ace->key.frame.ipv4.dport.match == VTSS_APPL_VCAP_ASR_TYPE_SPECIFIC) {
            ace.frame.ipv4.dport.low = ace.frame.ipv4.dport.high = appl_ace->key.frame.ipv4.dport.low;
        } else if (appl_ace->key.frame.ipv4.dport.match == VTSS_APPL_VCAP_ASR_TYPE_RANGE) {
            ace.frame.ipv4.dport.low = appl_ace->key.frame.ipv4.dport.low;
            ace.frame.ipv4.dport.high = appl_ace->key.frame.ipv4.dport.high;
        } else { // any
            ace.frame.ipv4.dport.low = 0;
            ace.frame.ipv4.dport.high = 0xFFFF;
        }

        // IPv4 TCP flags
        if (appl_ace->key.frame.ipv4.proto_type == VTSS_APPL_VCAP_AS_TYPE_SPECIFIC &&
            appl_ace->key.frame.ipv4.proto == 6) {
            _flag_import(appl_ace->key.frame.ipv4.tcp_flag.fin, ACE_FLAG_TCP_FIN, &ace);
            _flag_import(appl_ace->key.frame.ipv4.tcp_flag.syn, ACE_FLAG_TCP_SYN, &ace);
            _flag_import(appl_ace->key.frame.ipv4.tcp_flag.rst, ACE_FLAG_TCP_RST, &ace);
            _flag_import(appl_ace->key.frame.ipv4.tcp_flag.psh, ACE_FLAG_TCP_PSH, &ace);
            _flag_import(appl_ace->key.frame.ipv4.tcp_flag.ack, ACE_FLAG_TCP_ACK, &ace);
            _flag_import(appl_ace->key.frame.ipv4.tcp_flag.urg, ACE_FLAG_TCP_URG, &ace);
        } else {
            _flag_import(MESA_VCAP_BIT_ANY, ACE_FLAG_TCP_FIN, &ace);
            _flag_import(MESA_VCAP_BIT_ANY, ACE_FLAG_TCP_SYN, &ace);
            _flag_import(MESA_VCAP_BIT_ANY, ACE_FLAG_TCP_RST, &ace);
            _flag_import(MESA_VCAP_BIT_ANY, ACE_FLAG_TCP_PSH, &ace);
            _flag_import(MESA_VCAP_BIT_ANY, ACE_FLAG_TCP_ACK, &ace);
            _flag_import(MESA_VCAP_BIT_ANY, ACE_FLAG_TCP_URG, &ace);
        }

        ace.type = MESA_ACE_TYPE_IPV4;
        break;

    case VTSS_APPL_ACL_ACE_FRAME_TYPE_IPV6:
        // IPv6 DMAC
        if (_dmac_import(appl_ace->key.frame.dmac_type, &ace) != VTSS_RC_OK) {
            T_D("_dmac_import()");
            return VTSS_APPL_ACL_ERROR_PARM;
        }

        // IPv6 next header
        if (appl_ace->key.frame.ipv6.next_header_type == VTSS_APPL_VCAP_AS_TYPE_SPECIFIC) {
            ace.frame.ipv6.proto.value = appl_ace->key.frame.ipv6.next_header;
            ace.frame.ipv6.proto.mask = 0xFF;
        } else { // any
            ace.frame.ipv6.proto.value = ace.frame.ipv6.proto.mask = 0;
        }

        // IPv6 flags
        ace.frame.ipv6.ttl = (mesa_ace_bit_t) appl_ace->key.frame.ipv6.ipv6_flag.ttl;

        // IPv6 SIP
        memcpy(&ace.frame.ipv6.sip, &appl_ace->key.frame.ipv6.sip, sizeof(ace.frame.ipv6.sip));

        // IPv6 ICMP
        if (appl_ace->key.frame.ipv6.next_header_type == VTSS_APPL_VCAP_AS_TYPE_SPECIFIC &&
            appl_ace->key.frame.ipv6.next_header == 58) {
            if (appl_ace->key.frame.ipv6.icmp.type_match == VTSS_APPL_VCAP_AS_TYPE_SPECIFIC) {
                ace.frame.ipv6.data.value[0] = appl_ace->key.frame.ipv6.icmp.type;
                ace.frame.ipv6.data.mask[0] = 0xFF;
            } else { // any
                ace.frame.ipv6.data.value[0] =  ace.frame.ipv6.data.mask[0] = 0;
            }
            if (appl_ace->key.frame.ipv6.icmp.code_match == VTSS_APPL_VCAP_AS_TYPE_SPECIFIC) {
                ace.frame.ipv6.data.value[1] = appl_ace->key.frame.ipv6.icmp.code;
                ace.frame.ipv6.data.mask[1] = 0xFF;
            } else { // any
                ace.frame.ipv6.data.value[1] =  ace.frame.ipv6.data.mask[1] = 0;
            }
        } else {
            memset(&ace.frame.ipv6.data, 0, sizeof(ace.frame.ipv6.data));
        }

        // IPv6 TCP/UDP source port
        ace.frame.ipv6.sport.in_range = 1;
        if (appl_ace->key.frame.ipv6.sport.match == VTSS_APPL_VCAP_ASR_TYPE_SPECIFIC) {
            ace.frame.ipv6.sport.low = ace.frame.ipv6.sport.high = appl_ace->key.frame.ipv6.sport.low;
        } else if (appl_ace->key.frame.ipv6.sport.match == VTSS_APPL_VCAP_ASR_TYPE_RANGE) {
            ace.frame.ipv6.sport.low = appl_ace->key.frame.ipv6.sport.low;
            ace.frame.ipv6.sport.high = appl_ace->key.frame.ipv6.sport.high;
        } else { // any
            ace.frame.ipv6.sport.low = 0;
            ace.frame.ipv6.sport.high = 0xFFFF;
        }

        // IPv6 TCP/UDP destination port
        ace.frame.ipv6.dport.in_range = 1;
        if (appl_ace->key.frame.ipv6.dport.match == VTSS_APPL_VCAP_ASR_TYPE_SPECIFIC) {
            ace.frame.ipv6.dport.low = ace.frame.ipv6.dport.high = appl_ace->key.frame.ipv6.dport.low;
        } else if (appl_ace->key.frame.ipv6.dport.match == VTSS_APPL_VCAP_ASR_TYPE_RANGE) {
            ace.frame.ipv6.dport.low = appl_ace->key.frame.ipv6.dport.low;
            ace.frame.ipv6.dport.high = appl_ace->key.frame.ipv6.dport.high;
        } else { // any
            ace.frame.ipv6.dport.low = 0;
            ace.frame.ipv6.dport.high = 0xFFFF;
        }

        // IPv6 TCP flags
        if (appl_ace->key.frame.ipv6.next_header_type == VTSS_APPL_VCAP_AS_TYPE_SPECIFIC &&
            appl_ace->key.frame.ipv6.next_header == 6) {
            ace.frame.ipv6.tcp_fin = (mesa_ace_bit_t) appl_ace->key.frame.ipv6.tcp_flag.fin;
            ace.frame.ipv6.tcp_syn = (mesa_ace_bit_t) appl_ace->key.frame.ipv6.tcp_flag.syn;
            ace.frame.ipv6.tcp_rst = (mesa_ace_bit_t) appl_ace->key.frame.ipv6.tcp_flag.rst;
            ace.frame.ipv6.tcp_psh = (mesa_ace_bit_t) appl_ace->key.frame.ipv6.tcp_flag.psh;
            ace.frame.ipv6.tcp_ack = (mesa_ace_bit_t) appl_ace->key.frame.ipv6.tcp_flag.ack;
            ace.frame.ipv6.tcp_urg = (mesa_ace_bit_t) appl_ace->key.frame.ipv6.tcp_flag.urg;
        } else {
            ace.frame.ipv6.tcp_fin = MESA_ACE_BIT_ANY;
            ace.frame.ipv6.tcp_syn = MESA_ACE_BIT_ANY;
            ace.frame.ipv6.tcp_rst = MESA_ACE_BIT_ANY;
            ace.frame.ipv6.tcp_psh = MESA_ACE_BIT_ANY;
            ace.frame.ipv6.tcp_ack = MESA_ACE_BIT_ANY;
            ace.frame.ipv6.tcp_urg = MESA_ACE_BIT_ANY;
        }

        ace.type = MESA_ACE_TYPE_IPV6;
        break;

    default:
        T_D("invalid frame type %u", appl_ace->key.frame.frame_type);
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    /* Set ACE configuration */
    ace.id = ace_id;
    rc = acl_mgmt_ace_add(ACL_USER_STATIC, appl_ace->next_ace_id, &ace);

    T_D("exit: rc = %u", rc);
    return rc;
}

/* Check ACL interface action parameters */
static BOOL _interface_action_params_invalid(
    IN const vtss_appl_acl_action_t     *const interface_action
)
{
    // policer_id
    if (interface_action->rate_limiter_id > ACL_MGMT_RATE_LIMITER_NO_END) {
        T_D("ACL policer_id out of range");
        return TRUE;
    }

    // Hit action egress : not supported
    if (interface_action->hit_action == VTSS_APPL_ACL_HIT_ACTION_EGRESS) {
        T_D("Hit action egress not supported");
        return TRUE;
    }

    return FALSE;
}

/* Export ACL interface action configuration */
static mesa_rc _interface_action_export(
    IN  const acl_action_t          *const action,
    OUT vtss_appl_acl_action_t      *const interface_action
)
{
    mesa_rc rc = VTSS_APPL_ACL_ERROR_PARM;

    T_D("enter");

    /* Check illegal parameters */
    if (action == NULL || interface_action == NULL) {
        T_E("Input parameter is NULL");
        return rc;
    }

    /* Clear application configuration database */
    memset(interface_action, 0, sizeof(*interface_action));

    // policer
    interface_action->rate_limiter_id = action->policer == ACL_MGMT_RATE_LIMITER_NONE ? VTSS_APPL_ACL_POLICER_DISABLED : ipolicer2upolicer(action->policer);

    // Mirror
    interface_action->mirror = action->mirror;

    // Logging
    interface_action->logging = action->logging;

    // Shutdown
    interface_action->shutdown = action->shutdown;

    T_D("exit");
    return VTSS_RC_OK;
}

/* Import ACL interface action configuration */
static mesa_rc _interface_action_import(
    IN  const vtss_appl_acl_action_t    *const interface_action,
    OUT acl_action_t                    *const action
)
{
    mesa_rc rc = VTSS_APPL_ACL_ERROR_PARM;

    T_D("enter");

    /* Check illegal parameters */
    if (action == NULL || interface_action == NULL) {
        T_E("Input parameter is NULL");
        return rc;
    }
    if (_interface_action_params_invalid(interface_action)) {
        T_D("exit: Illegal parameters");
        return rc;
    }

    // ipolicer
    action->policer = interface_action->rate_limiter_id == VTSS_APPL_ACL_POLICER_DISABLED ? ACL_MGMT_RATE_LIMITER_NONE : upolicer2ipolicer(interface_action->rate_limiter_id);

    // Mirror
    action->mirror = interface_action->mirror;

    // Logging
    action->logging = interface_action->logging;

    // Shutdown
    action->shutdown = interface_action->shutdown;

    T_D("exit");

    return VTSS_RC_OK;
}

static mesa_rc _get_nth(
    IN  vtss_isid_t             isid,
    IN  u32                     precedence,
    IN  BOOL                    b_next,
    OUT acl_user_t              *user_id,
    OUT acl_entry_conf_t        *conf,
    OUT mesa_ace_counter_t      *counter
)
{
    mesa_rc     rc = VTSS_RC_OK;

    T_D("enter, isid: %d, precedence: %u, b_next: %s", isid, precedence, b_next ? "TRUE" : "FALSE");

    /* Check stack role */
    if (acl_mgmt_isid_invalid(isid)) {
        T_D("exit - Wrong stack role");
        return VTSS_APPL_ACL_ERROR_STACK_STATE;
    }

    if (msg_switch_is_local(isid)) {
        isid = VTSS_ISID_LOCAL;
    }

    rc = acl_list_ace_get_nth(isid, precedence, b_next, conf, counter);

    /* Restore independent ACE ID */
    *user_id = (acl_user_t) ACL_USER_ID_GET(conf->id);
    conf->id = ACL_ACE_ID_GET(conf->id);

    T_D("exit, rc = %u", rc);
    return rc;
}

static mesa_rc _precedence_itr(
    const u32   *const prev_precedence,
    u32         *const next_precedence
)
{
    acl_entry_conf_t        ace;
    acl_user_t    acl_user;
    vtss_isid_t             isid;
    u32                     precedence;

    /* Check illegal parameter */
    if (next_precedence == NULL) {
        T_W("next_precedence == NULL");
        return VTSS_RC_ERROR;
    }

    precedence = prev_precedence ? *prev_precedence : 0;
    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; ++isid) {
        if (_get_nth(isid, precedence, TRUE, &acl_user, &ace, NULL) == VTSS_RC_OK) {
            *next_precedence = precedence + 1;
            return VTSS_RC_OK;
        }
    }

    return VTSS_RC_ERROR;
}

uint32_t AclCapAceIdMax::get()
{
    return ACL_MGMT_ACE_ID_END;
}
uint32_t AclCapPolicyIdMax::get()
{
    return ACL_MGMT_POLICY_NO_MAX;
}
uint32_t AclCapRateLimiterIdMax::get()
{
    return ipolicer2upolicer(ACL_MGMT_RATE_LIMITER_NO_END - 1);
}
uint32_t AclCapEvcPolicerIdMax::get()
{
    return 0;
}

/**
 * \brief Get ACL capabilities to see what supported or not
 *
 * \param cap [OUT] ACL capabilities
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_capabilities_get(
    vtss_appl_acl_capabilities_t    *const cap
)
{
    if (cap == NULL) {
        T_W("cap == NULL");
        return VTSS_RC_ERROR;
    }

    cap->acl_ace_id_max          = ACL_MGMT_ACE_ID_END;
    cap->acl_policy_id_max       = ACL_MGMT_POLICY_NO_MAX;
    cap->acl_rate_limiter_id_max = ipolicer2upolicer(ACL_MGMT_RATE_LIMITER_NO_END - 1);
    cap->acl_evc_policer_id_max  = 0;
    cap->action_evc_policer_supported = FALSE;
    cap->rate_limiter_bit_rate_supported = TRUE;
    cap->action_mirror_supported = TRUE;
    cap->ace_second_lookup_supported = (fast_cap(MESA_CAP_ACL_KEY_LOOKUP) ? TRUE : FALSE);
    cap->action_multiple_redirect_ports_supported = TRUE;
    cap->ace_multiple_ingress_ports_supported     = TRUE;
    cap->ace_egress_port_supported                = TRUE;
    cap->ace_vlan_tagged_supported = TRUE;

    return VTSS_RC_OK;
}

/**
 * \brief ACL rate limiter iterate function,
 *
 * \param prev_rate_limiter_id [IN]  previous rate limter id.
 * \param next_rate_limiter_id [OUT] next rate limiter id.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_rate_limiter_itr(
    const mesa_acl_policer_no_t     *const prev_rate_limiter_id,
    mesa_acl_policer_no_t           *const next_rate_limiter_id
)
{
    vtss::expose::snmp::IteratorComposeRange<mesa_acl_policer_no_t>     itr(ipolicer2upolicer(ACL_MGMT_RATE_LIMITER_NO_START),
                                                                            ipolicer2upolicer(ACL_MGMT_RATE_LIMITER_NO_END));

    return itr(prev_rate_limiter_id, next_rate_limiter_id);
}

/**
 * \brief Get ACL rate limiter configuration
 *
 * \param rate_limiter_id [IN]  Rate limiter ID
 * \param conf            [OUT] Rate limiter configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_rate_limiter_get(
    mesa_acl_policer_no_t                   rate_limiter_id,
    vtss_appl_acl_config_rate_limiter_t     *const conf
)
{
    mesa_rc     rc;

    if (rate_limiter_id < ipolicer2upolicer(ACL_MGMT_RATE_LIMITER_NO_START) ||
        rate_limiter_id > ipolicer2upolicer(ACL_MGMT_RATE_LIMITER_NO_END - 1)) {
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    if (conf == NULL) {
        T_E("conf == NULL");
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    memset(conf, 0, sizeof(vtss_appl_acl_config_rate_limiter_t));

    rc = acl_mgmt_policer_conf_get(upolicer2ipolicer(rate_limiter_id), conf);

    return rc;
}

/**
 * \brief Set ACL rate limiter configuration
 *
 * \param rate_limiter_id [IN] Rate limiter ID
 * \param conf            [IN] Rate limiter configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_rate_limiter_set(
    mesa_acl_policer_no_t                           rate_limiter_id,
    const vtss_appl_acl_config_rate_limiter_t       *const conf
)
{
    if (rate_limiter_id < ipolicer2upolicer(ACL_MGMT_RATE_LIMITER_NO_START) ||
        rate_limiter_id >= ipolicer2upolicer(ACL_MGMT_RATE_LIMITER_NO_END)) {
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    if (conf == NULL) {
        T_E("conf == NULL");
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    if (conf->bit_rate_enable) {
        if ((conf->bit_rate % ACL_MGMT_BIT_RATE_GRANULARITY) ||
            conf->bit_rate > ACL_MGMT_BIT_RATE_MAX) {
            T_D("Illegal parameter: bit_rate = %u", conf->bit_rate);
            return VTSS_APPL_ACL_ERROR_PARM;
        }
    } else if (conf->packet_rate > ACL_MGMT_PACKET_RATE_MAX) {
        T_D("Illegal parameter: packet_rate = %u", conf->packet_rate);
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    if ((conf->packet_rate % ACL_MGMT_PACKET_RATE_GRANULARITY) &&
        (ACL_MGMT_PACKET_RATE_GRANULARITY_SMALL_RANGE == 0 ||
         conf->packet_rate > ACL_MGMT_PACKET_RATE_GRANULARITY)) {
        T_D("Illegal parameter: packet_rate = %u", conf->packet_rate);
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    return acl_mgmt_policer_conf_set(upolicer2ipolicer(rate_limiter_id), conf);
}

/**
 * \brief Iterate function of ACE config table.
 *
 * \param prev_ace_id [IN]  previous ACE ID.
 * \param next_ace_id [OUT] next ACE ID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_ace_itr(
    const vtss_appl_acl_ace_id_t    *const prev_ace_id,
    vtss_appl_acl_ace_id_t          *const next_ace_id
)
{
    mesa_rc             rc;
    mesa_ace_id_t       ace_idx;
    acl_entry_conf_t    conf;

    /* Check illegal parameters */
    if (next_ace_id == NULL) {
        T_W("Input parameter is NULL");
        return VTSS_RC_ERROR;
    }

    if (prev_ace_id && (*prev_ace_id >= ACL_MGMT_ACE_ID_END)) {
        return VTSS_RC_ERROR;
    }

    /* Get available index */
    rc = VTSS_RC_ERROR;

    // Try a soft touch to check if any existing ACE
    if (acl_mgmt_ace_get(ACL_USER_STATIC, ACL_MGMT_ACE_ID_NONE, &conf, NULL, TRUE) == VTSS_RC_OK) {
        conf.id = ACL_MGMT_ACE_ID_NONE;
        if (prev_ace_id) { // getnext
            for (ace_idx = *prev_ace_id + 1; ace_idx <= ACL_MGMT_ACE_ID_END; ace_idx++) {
                if (acl_mgmt_ace_get(ACL_USER_STATIC, ace_idx, &conf, NULL, FALSE) == VTSS_RC_OK) {
                    // Found it
                    *next_ace_id = ace_idx;
                    rc = VTSS_RC_OK;
                    break;
                }
            }
        } else { // getfirst
            for (ace_idx = ACL_MGMT_ACE_ID_START; ace_idx <= ACL_MGMT_ACE_ID_END; ace_idx++) {
                if (acl_mgmt_ace_get(ACL_USER_STATIC, ace_idx, &conf, NULL, FALSE) == VTSS_RC_OK) {
                    // Found it
                    *next_ace_id = conf.id;
                    rc = VTSS_RC_OK;
                    break;
                }
            }
        }
    }

    return rc;
}

/**
 * \brief Get ACL ACE configuration.
 *
 * \param ace_id [IN]  ACE ID.
 * \param conf   [OUT] ACE configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_ace_get(
    vtss_appl_acl_ace_id_t          ace_id,
    vtss_appl_acl_config_ace_t      *const conf
)
{
    /* Check illegal parameters */
    if (ace_id < ACL_MGMT_ACE_ID_START || ace_id > ACL_MGMT_ACE_ID_END) {
        T_W("invalid ace_id %u", ace_id);
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    if (conf == NULL) {
        T_E("conf == NULL");
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    return _ace_export(ace_id, conf);
}

/**
 * \brief Set ACL ACE configuration.
 *
 * \param ace_id [IN]  ACE ID.
 * \param conf   [OUT] ACE configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_ace_set(
    vtss_appl_acl_ace_id_t              ace_id,
    const vtss_appl_acl_config_ace_t    *const conf
)
{
    /* Check illegal parameters */
    if (ace_id < ACL_MGMT_ACE_ID_START || ace_id > ACL_MGMT_ACE_ID_END) {
        T_W("invalid ace_id %u", ace_id);
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    if (conf == NULL) {
        T_E("conf == NULL");
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    return _ace_import(ace_id, conf, FALSE);
}

/**
 * \brief Add ACL ACE configuration
 *
 * \param ace_id [IN]  ACE ID.
 * \param conf   [OUT] ACE configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_ace_add(
    vtss_appl_acl_ace_id_t              ace_id,
    const vtss_appl_acl_config_ace_t    *const conf
)
{
    /* Check illegal parameters */
    if (ace_id < ACL_MGMT_ACE_ID_START || ace_id > ACL_MGMT_ACE_ID_END) {
        T_W("invalid ace_id %u", ace_id);
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    if (conf == NULL) {
        T_E("conf == NULL");
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    return _ace_import(ace_id, conf, TRUE);
}

/**
 * \brief Delete ACL ACE configuration.
 *
 * \param ace_id [IN] ACE ID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_ace_del(
    vtss_appl_acl_ace_id_t      ace_id
)
{
    /* Check illegal parameter */
    if (ace_id < ACL_MGMT_ACE_ID_START || ace_id > ACL_MGMT_ACE_ID_END) {
        T_W("invalid ace_id %u", ace_id);
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    return acl_mgmt_ace_del(ACL_USER_STATIC, ace_id);
}

/**
 * \brief Add ACL ACE default configuration.
 *
 * \param ace_id [IN] ACE ID.
 * \param conf   [IN] ACE configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_ace_default(
    vtss_appl_acl_ace_id_t      *const ace_id,
    vtss_appl_acl_config_ace_t  *const conf
)
{
    *ace_id = 1;
    return _ace_export(0, conf);
}

/**
 * \brief Iterate function of interface config table.
 *
 * \param prev_ifindex [IN]  ifindex of previous port.
 * \param next_ifindex [OUT] ifindex of next port.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_interface_itr(
    const vtss_ifindex_t    *prev_ifindex,
    vtss_ifindex_t          *next_ifindex
)
{
    return vtss_appl_iterator_ifindex_front_port(prev_ifindex, next_ifindex);
}

/**
 * \brief Get ACL interface configuration.
 *
 * \param ifindex [IN]  ifindex of port.
 * \param conf    [OUT] The data point of configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_interface_get(
    vtss_ifindex_t                      ifindex,
    vtss_appl_acl_config_interface_t    *const conf
)
{
    mesa_rc            rc;
    vtss_ifindex_elm_t ife;
    acl_port_conf_t    port_conf;
    mesa_port_no_t     iport;
    u32                action_port_list_cnt;
    u32                port_count = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    /* Check illegal parameters */
    if (conf == NULL) {
        T_E("Input parameter is NULL");
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    /* get isid/iport from ifindex and validate them */
    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    /* Clear application configuration database */
    memset(conf, 0, sizeof(*conf));

    /* Get ACL port configuation */
    vtss_clear(port_conf);
    rc = acl_mgmt_port_conf_get((mesa_port_no_t)ife.ordinal, &port_conf);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    // Export ACL application port configuration
    rc = _interface_action_export(&port_conf.action, &conf->action);
    if (rc != VTSS_RC_OK) {
        T_D("exit: fail to export interface action");
        return rc;
    }

    // Port policy No.
    conf->policy_id = port_conf.policy_no;

    // get port count
    action_port_list_cnt = 0;
    for (iport = 0; iport < port_count; iport++) {
        if (port_conf.action.port_list[iport]) {
            action_port_list_cnt++;
        }
    }

    switch (port_conf.action.port_action) {
    case MESA_ACL_PORT_ACTION_NONE:
        // Port hit action
        conf->action.hit_action = VTSS_APPL_ACL_HIT_ACTION_PERMIT;
        break;

    case MESA_ACL_PORT_ACTION_FILTER:
        if (action_port_list_cnt) {
            if (_stackable_set_by_port_list(ife.isid, port_conf.action.port_list, &(conf->action.egress_port)) == FALSE) {
                T_W("_stackable_set_by_port_list(%u, egress_port)", ife.isid);
                return VTSS_RC_ERROR;
            }
            // Port hit action
            conf->action.hit_action = VTSS_APPL_ACL_HIT_ACTION_EGRESS;
        } else {
            // Port hit action
            conf->action.hit_action = VTSS_APPL_ACL_HIT_ACTION_DENY;
        }
        break;

    case MESA_ACL_PORT_ACTION_REDIR:
        if (action_port_list_cnt) {
            if (_stackable_set_by_port_list(ife.isid, port_conf.action.port_list, &(conf->action.redirect_port)) == FALSE) {
                T_W("_stackable_set_by_port_list(%u, redirect_port)", ife.isid);
                return VTSS_RC_ERROR;
            }
            // Port hit action
            conf->action.hit_action = VTSS_APPL_ACL_HIT_ACTION_REDIRECT;
        } else {
            // Port hit action
            conf->action.hit_action = VTSS_APPL_ACL_HIT_ACTION_DENY;
        }
        break;

    default:
        T_D("invalid port action : %u", port_conf.action.port_action);
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Set ACL interface configuration.
 *
 * \param ifindex [IN] ifindex of port.
 * \param conf    [IN] The data point of configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_interface_set(
    vtss_ifindex_t                              ifindex,
    const vtss_appl_acl_config_interface_t      *const conf
)
{
    mesa_rc             rc;
    vtss_ifindex_elm_t  ife;
    acl_port_conf_t     port_conf;
    vtss_isid_t         isid;
    mesa_port_no_t      iport;
    u32                 port_count = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    T_D("enter");

    /* Check illegal parameters */
    if (conf == NULL) {
        T_E("Input parameter is NULL");
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    if (conf->policy_id > ACL_MGMT_POLICY_NO_MAX) {
        T_W("exit: Illegal parameter: upolicy = %u", conf->policy_id);
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    /* get isid/iport from ifindex and validate them */
    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    /* Get original ACL port configuration */
    rc = acl_mgmt_port_conf_get((mesa_port_no_t)ife.ordinal, &port_conf);
    if (rc != VTSS_RC_OK) {
        T_D("exit: fail to get port config");
        return rc;
    }

    // Import ACL applicationport configuration
    rc = _interface_action_import(&(conf->action), &(port_conf.action));
    if (rc != VTSS_RC_OK) {
        T_D("exit: fail to import interface action");
        return rc;
    }

    // Port policy No.
    port_conf.policy_no = conf->policy_id;

    // reset all ports
    memset(port_conf.action.port_list, 0, sizeof(port_conf.action.port_list));

    switch (conf->action.hit_action) {
    case VTSS_APPL_ACL_HIT_ACTION_PERMIT:
        port_conf.action.port_action = MESA_ACL_PORT_ACTION_NONE;
        memset(port_conf.action.port_list, 1, sizeof(port_conf.action.port_list));
        break;

    case VTSS_APPL_ACL_HIT_ACTION_DENY:
        port_conf.action.port_action = MESA_ACL_PORT_ACTION_FILTER;
        memset(port_conf.action.port_list, 0, sizeof(port_conf.action.port_list));
        break;

    case VTSS_APPL_ACL_HIT_ACTION_REDIRECT:
        port_conf.action.port_action = MESA_ACL_PORT_ACTION_REDIR;
        if (_port_list_get_by_stackable(&(conf->action.redirect_port), &isid, port_conf.action.port_list)) {
            BOOL found_redirect_port = FALSE;
            if (isid) {
                if (msg_switch_is_local(isid) == FALSE) {
                    T_D("isid %u is not local isid", isid);
                    return VTSS_APPL_ACL_ERROR_PARM;
                }
            }
            for (iport = 0; iport < port_count; iport++) {
                if (port_conf.action.port_list[iport]) {
                    // Found one redirect port
                    found_redirect_port = TRUE;
                    break;
                }
            }
            if (!found_redirect_port) {
                // No redirect port is found (need one redirect port, at least)
                return VTSS_APPL_ACL_ERROR_PARM;
            }
        } else {
            return VTSS_APPL_ACL_ERROR_PARM;
        }
        break;

    default:
        T_D("invalid hit action : %u", conf->action.hit_action);
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    /* Set new ACL port configuration */
    rc = acl_mgmt_port_conf_set((mesa_port_no_t)ife.ordinal, &port_conf);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    T_D("exit: rc = %u", rc);
    return rc;
}

/**
 * \brief ACE precedence iterate function
 *
 * \param prev_precedence [IN]  previous precedence.
 * \param next_precedence [OUT] next precedence.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_ace_precedence_itr(
    const u32   *const prev_precedence,
    u32         *const next_precedence
)
{
    mesa_rc                 rc;
    acl_entry_conf_t        ace;
    acl_user_t    acl_user;

    /* Check illegal parameter */
    if (next_precedence == NULL) {
        T_W("next_precedence == NULL");
        return VTSS_RC_ERROR;
    }

    rc = _get_nth(VTSS_ISID_GLOBAL, prev_precedence ? *prev_precedence : 0, TRUE, &acl_user, &ace, NULL);
    if (rc == VTSS_RC_OK) {
        *next_precedence = prev_precedence ? *prev_precedence + 1 : 1;
    }

    return rc;
}

/**
 * \brief Get ACL ACE precedence status.
 *
 * \param precedence [IN]  precedence.
 * \param status     [OUT] precedence tatus.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_ace_precedence_get(
    u32                                     precedence,
    vtss_appl_acl_config_ace_precedence_t   *const status
)
{
    mesa_rc                 rc;
    acl_entry_conf_t        ace;
    acl_user_t    acl_user;

    /* Check illegal parameter */
    if (status == NULL) {
        T_E("status == NULL");
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    /* Get ACE configuration */
    rc = _get_nth(VTSS_ISID_GLOBAL, precedence, FALSE, &acl_user, &ace, NULL);

    if (rc != VTSS_RC_OK) {
        return rc;
    }

    status->ace_id = ace.id;

    return VTSS_RC_OK;
}

/**
 * \brief ACE status iterate function, it is used to get first and
 *        get next indexes.
 *
 * \param prev_usid       [IN]  previous switch ID.
 * \param next_usid       [OUT] next switch ID.
 * \param prev_precedence [IN]  previous precedence.
 * \param next_precedence [OUT] next precedence.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_status_ace_itr(
    const vtss_usid_t   *const prev_usid,
    vtss_usid_t         *const next_usid,
    const u32           *const prev_precedence,
    u32                 *const next_precedence
)
{
    vtss::IteratorComposeN<vtss_usid_t, u32> itr(
        &vtss_appl_iterator_switch,
        &_precedence_itr);

    return itr(prev_usid, next_usid, prev_precedence, next_precedence);
}

/**
 * \brief ACE event status iterate function, it is used to get first and
 *        get next indexes.
 *
 * \param prev_usid       [IN]  previous switch ID.
 * \param next_usid       [OUT] next switch ID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_status_ace_event_itr(
    const vtss_usid_t   *const prev_usid,
    vtss_usid_t         *const next_usid
)
{
    return vtss_appl_iterator_switch(prev_usid, next_usid);
}

/**
 * \brief Get ACL ACE status.
 *
 * \param usid       [IN]  switch ID for user view (The value starts from 1).
 * \param precedence [IN]  precedence.
 * \param status     [OUT] ACE status.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_status_ace_get(
    vtss_usid_t                     usid,
    u32                             precedence,
    vtss_appl_acl_status_ace_t      *const status
)
{
    mesa_rc                 rc;
    acl_entry_conf_t        ace;
    acl_user_t    acl_user;
    vtss_isid_t             isid;

    /* Check illegal parameters */
    if (usid < VTSS_USID_START || usid >= VTSS_USID_END) {
        T_W("invalid usid %u", usid);
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    isid = topo_usid2isid(usid);

    if (! msg_switch_exists(isid)) {
        return VTSS_APPL_ACL_ERROR_ISID_NON_EXISTING;
    }

    if (status == NULL) {
        T_E("status == NULL");
        return VTSS_RC_ERROR;
    }

    /* Get ACE configuration */
    rc = _get_nth(isid, precedence, FALSE, &acl_user, &ace, NULL);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    if (acl_user >= ACL_USER_CNT) {
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    memset(status, 0, sizeof(vtss_appl_acl_status_ace_t));

    strncpy(status->acl_user, acl_mgmt_user_name_get(acl_user), VTSS_APPL_ACL_USER_MAX_LEN);

    status->ace_id   = ace.id;
    status->conflict = ace.conflict;

    return VTSS_RC_OK;
}

/**
 * \brief Iterate function of ACE hit count status table.
 *
 * \param prev_ace_id [IN]  previous ACE ID.
 * \param next_ace_id [OUT] next ACE ID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_status_ace_hit_count_itr(
    const vtss_appl_acl_ace_id_t    *const prev_ace_id,
    vtss_appl_acl_ace_id_t          *const next_ace_id
)
{
    return vtss_appl_acl_config_ace_itr(prev_ace_id, next_ace_id);
}

/**
 * \brief Get ACL ACE hit count.
 *
 * \param ace_id [IN]  ACE ID.
 * \param status [OUT] hit count status.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_status_ace_hit_count_get(
    vtss_appl_acl_ace_id_t                  ace_id,
    vtss_appl_acl_status_ace_hit_count_t    *const status
)
{
    acl_entry_conf_t    ace;

    /* Check illegal parameters */
    if (ace_id < ACL_MGMT_ACE_ID_START || ace_id > ACL_MGMT_ACE_ID_END) {
        T_W("invalid ace_id %u", ace_id);
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    if (status == NULL) {
        T_W("status == NULL");
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    return acl_mgmt_ace_get(ACL_USER_STATIC, ace_id, &ace, &(status->counter), FALSE);
}

/**
 * \brief Iterate function of interface config table.
 *
 * \param prev_ifindex [IN]  ifindex of previous port.
 * \param next_ifindex [OUT] ifindex of next port.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_status_interface_hit_count_itr(
    const vtss_ifindex_t    *prev_ifindex,
    vtss_ifindex_t          *next_ifindex
)
{
    return vtss_appl_acl_config_interface_itr(prev_ifindex, next_ifindex);
}

/**
 * \brief Get ACL interface hit count.
 *
 * \param ifindex [IN]  ifindex of port.
 * \param status  [OUT] hit count status.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_status_interface_hit_count_get(
    vtss_ifindex_t                                  ifindex,
    vtss_appl_acl_status_interface_hit_count_t      *const status
)
{
    vtss_ifindex_elm_t  ife;

    /* Check illegal parameters */
    if (status == NULL) {
        T_E("Input parameter is NULL");
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    /* get isid/iport from ifindex and validate them */
    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    return acl_mgmt_port_counter_get((mesa_port_no_t)ife.ordinal, &(status->counter));
}

/**
 * \brief Get ACL global control.
 *
 * \param control [OUT]: global control.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_control_globals_get(
    vtss_appl_acl_control_globals_t     *const control
)
{
    /* Check illegal parameter */
    if (control == NULL) {
        T_E("control == NULL");
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    control->clear_all_hit_count = FALSE;
    return VTSS_RC_OK;
}

/**
 * \brief Set ACL global control.
 *
 * \param control [IN] global control.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_control_globals_set(
    const vtss_appl_acl_control_globals_t   *const control
)
{
    /* Check illegal parameter */
    if (control == NULL) {
        T_E("control == NULL");
        return VTSS_RC_ERROR;
    }

    if (control->clear_all_hit_count == TRUE) {
        acl_mgmt_counters_clear();
    }

    return VTSS_RC_OK;
}

/**
 * \brief Iterate function of interface control table.
 *
 * \param prev_ifindex [IN]  ifindex of previous port.
 * \param next_ifindex [OUT] ifindex of next port.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_control_interface_itr(
    const vtss_ifindex_t    *prev_ifindex,
    vtss_ifindex_t          *next_ifindex
)
{
    return vtss_appl_iterator_ifindex_front_port_exist(prev_ifindex, next_ifindex);
}

/**
 * \brief Get ACL interface control.
 *
 * \param ifindex [IN]  ifindex of port.
 * \param control [OUT] interface control data.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_control_interface_get(
    vtss_ifindex_t                      ifindex,
    vtss_appl_acl_control_interface_t   *const control
)
{
    mesa_rc                 rc;
    vtss_ifindex_elm_t      ife;
    port_vol_conf_t         port_state_conf;

    /* Check illegal parameters */
    if (control == NULL) {
        T_E("Input parameter is NULL");
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    /* get isid/iport from ifindex and validate them */
    if (vtss_appl_ifindex_port_exist(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    /* Clear control database */
    memset(control, 0, sizeof(*control));

    /* Get ACL port configuation */
    memset(&port_state_conf, 0, sizeof(port_state_conf));

    rc = port_vol_conf_get(PORT_USER_ACL, ife.ordinal, &port_state_conf);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    // Port state
    control->state = !(port_state_conf.disable);

    return VTSS_RC_OK;
}

/**
 * \brief Set ACL interface control.
 *
 * \param ifindex [IN] ifindex of port.
 * \param control [IN] interface control data.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_control_interface_set(
    vtss_ifindex_t                              ifindex,
    const vtss_appl_acl_control_interface_t     *const control
)
{
    mesa_rc             rc;
    vtss_ifindex_elm_t  ife;
    port_vol_conf_t     port_state_conf;

    /* Check illegal parameters */
    if (control == NULL) {
        T_E("Input parameter is NULL");
        return VTSS_APPL_ACL_ERROR_PARM;
    }

    /* get isid/iport from ifindex and validate them */
    if (vtss_appl_ifindex_port_exist(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    rc = port_vol_conf_get(PORT_USER_ACL, ife.ordinal, &port_state_conf);
    if (rc != VTSS_RC_OK) {
        T_D("exit: fail to get port vol config");
        return rc;
    }

    // Port state
    port_state_conf.disable = !(control->state);

    /* Set new ACL port state configuration */
    rc = port_vol_conf_set(PORT_USER_ACL, ife.ordinal, &port_state_conf);

    return rc;
}

