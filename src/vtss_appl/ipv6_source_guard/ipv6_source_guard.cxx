/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "critd_api.h"
#include "qos_api.h"
#include "mgmt_api.h"
#include "port_api.h"
#include "ip_utils.hxx"

#include "vtss/appl/ipv6_source_guard.h"
#include "ipv6_source_guard_trace.h"
#include "ipv6_source_guard.h"
#include "ipv6_source_guard_icfg.h"
#include "vtss/appl/dhcp6_snooping.h"

#include "vtss/basics/expose/table-status.hxx"
#include "vtss/basics/memcmp-operator.hxx"
#include "vtss_common_iterator.hxx"
#include "vtss/appl/qos.h"

#ifdef VTSS_SW_OPTION_ICFG
#include "ipv6_source_guard_icfg.h"
#endif /* VTSS_SW_OPTION_ICFG */

#ifdef VTSS_SW_OPTION_WEB
#include "web_api.h"
#endif /* VTSS_SW_OPTION_WEB */

#if defined(VTSS_SW_OPTION_SYSLOG)
#include "syslog_api.h"
#endif  /* VTSS_SW_OPTION_SYSLOG */

#include "vtss_tftp_api.h"

/****************************************************************************/
/*  TRACE system                                                            */
/****************************************************************************/

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "ipv6_guard", "Ipv6 Source Guard"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define IPV6_SOURCE_GUARD_CRIT_ENTER() critd_enter(&ipv6_source_guard_crit, __FILE__, __LINE__)
#define IPV6_SOURCE_GUARD_CRIT_EXIT()  critd_exit( &ipv6_source_guard_crit, __FILE__, __LINE__)

/****************************************************************************/
/*  Global variables */
/****************************************************************************/

#define QCE_ID_BINDING_BASE     1
#define QCE_ID_SESSION_START    2
#define ACE_ID_PORT_DEFAULT     1
#define ACE_ID_BINDING_BASE     2

/* IPv6 Source Guard Global configuration */
static vtss_appl_ipv6_source_guard_global_config_t default_conf;

/* Critical region protection */
static critd_t ipv6_source_guard_crit;  // Critical region for global variables
/* Global ACL information */
static struct global_data_t {
    mesa_ace_id_t ace_port_id_default;
    vtss::Vector<mesa_qce_id_t> qce_id_free_list;
    vtss::Vector<mesa_qce_id_t> qce_id_used_list;
} global_data;

/* In order to compare variables in the relevant tables. */
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_ipv6_source_guard_port_config_info_t);
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_ipv6_source_guard_entry_qce_data_t);

bool operator < (vtss_appl_ipv6_source_guard_entry_index_t const& lhs, vtss_appl_ipv6_source_guard_entry_index_t const& rhs)
{
    if (lhs.ifindex < rhs.ifindex)
        return true;
    if (lhs.ifindex > rhs.ifindex)
        return false;
    if (lhs.ipv6_addr < rhs.ipv6_addr)
        return true;
    if (lhs.ipv6_addr > rhs.ipv6_addr)
        return false;

    return lhs.vlan_id < rhs.vlan_id;
}

bool operator != (vtss_appl_ipv6_source_guard_entry_index_t const& lhs, vtss_appl_ipv6_source_guard_entry_index_t const& rhs)
{
    if (lhs.ifindex != rhs.ifindex)
        return true;
    if (lhs.ipv6_addr != rhs.ipv6_addr)
        return true;

    return lhs.vlan_id != rhs.vlan_id;
}

vtss::expose::TableStatus<
    vtss::expose::ParamKey<vtss_ifindex_t>,
    vtss::expose::ParamVal<vtss_appl_ipv6_source_guard_port_config_info_t *>
> ipv6_source_guard_port_configuration("ipv6_source_guard_port_configuration", VTSS_MODULE_ID_IPV6_SOURCE_GUARD);

vtss::expose::TableStatus<
    vtss::expose::ParamKey<vtss_appl_ipv6_source_guard_entry_index_t>,
    vtss::expose::ParamVal<vtss_appl_ipv6_source_guard_entry_qce_data_t *>
> ipv6_source_guard_static_entries("ipv6_source_guard_static_entries", VTSS_MODULE_ID_IPV6_SOURCE_GUARD);

vtss::expose::TableStatus<
    vtss::expose::ParamKey<vtss_appl_ipv6_source_guard_entry_index_t>,
    vtss::expose::ParamVal<vtss_appl_ipv6_source_guard_entry_qce_data_t *>
> ipv6_source_guard_dynamic_entries("ipv6_source_guard_dynamic_entries", VTSS_MODULE_ID_IPV6_SOURCE_GUARD);

/*************************************************************************
** Static Functions Declarations
*************************************************************************/
static mesa_rc ipv6_source_guard_port_conf_itr(
    const vtss_ifindex_t    *const prev,
    vtss_ifindex_t          *const next);

static mesa_rc  ipv6_source_guard_static_entry_itr(
    const vtss_appl_ipv6_source_guard_entry_index_t  *const prev,
    vtss_appl_ipv6_source_guard_entry_index_t        *const next); 

static mesa_rc ipv6_source_guard_dynamic_entry_itr(
    const vtss_appl_ipv6_source_guard_entry_index_t  *const prev,
    vtss_appl_ipv6_source_guard_entry_index_t        *const next);

static mesa_rc ipv6_source_guard_global_config_get(
    vtss_appl_ipv6_source_guard_global_config_t    *const conf);

static mesa_rc ipv6_source_guard_global_config_set(
    const vtss_appl_ipv6_source_guard_global_config_t  *const conf);

static mesa_rc ipv6_source_guard_port_config_get(
    vtss_ifindex_t  ifindex,
    vtss_appl_ipv6_source_guard_port_config_t  *const port_conf);

static mesa_rc ipv6_source_guard_port_config_set(
    vtss_ifindex_t  ifindex,
    const vtss_appl_ipv6_source_guard_port_config_t   *const port_conf);

static mesa_rc ipv6_source_guard_static_entry_data_get(
    const vtss_appl_ipv6_source_guard_entry_index_t  *const static_index,
    vtss_appl_ipv6_source_guard_entry_data_t         *const static_data);

static mesa_rc ipv6_source_guard_static_entry_set(
    const vtss_appl_ipv6_source_guard_entry_index_t  *const static_index,
    const vtss_appl_ipv6_source_guard_entry_data_t   *const static_data);

static mesa_rc ipv6_source_guard_static_entry_del(
    const vtss_appl_ipv6_source_guard_entry_index_t  *const static_index);

static mesa_rc ipv6_source_guard_dynamic_entry_data_get(
    const vtss_appl_ipv6_source_guard_entry_index_t *const dynamic_index,
    vtss_appl_ipv6_source_guard_entry_data_t        *const dynamic_data);

static mesa_rc ipv6_source_guard_dynamic_entry_set(
    vtss_appl_ipv6_source_guard_entry_index_t index,
    vtss_appl_ipv6_source_guard_entry_qce_data_t *data);

static mesa_rc ipv6_source_guard_dynamic_entry_del(
    vtss_appl_ipv6_source_guard_entry_index_t index);

/*************************************************************************
** Utility Functions
*************************************************************************/

/* 
 * Get total number of entries. 
 */
static int get_total_number_of_entries() 
{
    return ipv6_source_guard_static_entries.size() + ipv6_source_guard_dynamic_entries.size();
}

/*
 * Get QCE id for binding entry.
 */
static mesa_qce_id_t get_free_qce_id() 
{
    
    IPV6_SOURCE_GUARD_CRIT_ENTER();
    if (global_data.qce_id_free_list.size() > 0) {
        mesa_qce_id_t qce_id = global_data.qce_id_free_list.back();
        global_data.qce_id_free_list.pop_back();
        IPV6_SOURCE_GUARD_CRIT_EXIT();
        return qce_id;
    }
    IPV6_SOURCE_GUARD_CRIT_EXIT();

return VTSS_APPL_QOS_QCE_ID_NONE;
}

/*
 * Add QCE id back to free pool.
 */
static void release_qce_id(mesa_qce_id_t qce_id)
{
    if (qce_id == VTSS_APPL_QOS_QCE_ID_NONE) {
        return;
    }

    IPV6_SOURCE_GUARD_CRIT_ENTER();
    auto it = find(global_data.qce_id_free_list.begin(), global_data.qce_id_free_list.end(), qce_id);
    if (it == global_data.qce_id_free_list.end()) {
        global_data.qce_id_free_list.push_back(qce_id);
    }

    it = find(global_data.qce_id_used_list.begin(), global_data.qce_id_used_list.end(), qce_id);
    if (it != global_data.qce_id_used_list.end()) {
        global_data.qce_id_used_list.erase(it);
    }
    IPV6_SOURCE_GUARD_CRIT_EXIT();
} 

/*
 * Get port number from interface index.
 */
static mesa_port_no_t get_port_from_ifindex(vtss_ifindex_t ifindex)
{
    vtss_ifindex_elm_t ife;

    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return MESA_PORT_NO_NONE;
    }

    return ife.ordinal;
}

/*
 * To check if port is enabled. 
 */
static mesa_rc port_enabled_check(vtss_ifindex_t ifindex) 
{
    vtss_appl_ipv6_source_guard_port_config_info_t port_conf;
    bool enabled;

    if (ipv6_source_guard_port_configuration.get(ifindex, &port_conf) != MESA_RC_OK) {
        T_D("Could not get port configuration, exit");
        return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
    }

    enabled = port_conf.conf.enabled;

    if (!enabled) {
        T_D("Port is not enabled, exit");
        return IPV6_SOURCE_GUARD_ERROR_DISABLED;
    }

    return MESA_RC_OK;
}

static mesa_rc ipv6_source_guard_global_port_enabled_check(vtss_ifindex_t ifindex) 
{
    bool enabled;

    T_D("enter");

    /* Check if module is globally enabled. */
    IPV6_SOURCE_GUARD_CRIT_ENTER();
    enabled = default_conf.enabled;
    IPV6_SOURCE_GUARD_CRIT_EXIT();

    if (!enabled) {
        T_D("Module is not enabled, exit");
        return IPV6_SOURCE_GUARD_ERROR_DISABLED;
    }

    /* Check if port is enabled. */
    if (port_enabled_check(ifindex) != MESA_RC_OK) {
        return IPV6_SOURCE_GUARD_ERROR_DISABLED;
    }

    T_D("exit");
    return MESA_RC_OK;
}

/* IPV6_SOURCE_GUARD error text */
const char *ipv6_source_guard_error_txt(mesa_rc rc)
{
    switch (rc) {
    case IPV6_SOURCE_GUARD_ERROR_INV_PARAMETER:
        return "IPv6 source guard: Invalid parameter submitted.";

    case IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL:
        return "IPv6 source guard: Operation did not succeed.";

    case IPV6_SOURCE_GUARD_ERROR_PORT_CONFIGURE:
        return "IPv6 source guard: Port could not be configured.";

    case IPV6_SOURCE_GUARD_ERROR_DEFAULT_CONFIGURE:
        return "IPv6 source guard: Source guard could not be configured.";

    case IPV6_SOURCE_GUARD_ERROR_DEFAULT_ACE_RULE:
        return "IPv6 source guard: Default ace rule could not be created.";

    case IPV6_SOURCE_GUARD_ERROR_DEFAULT_ACE_RULE_DEL:
        return "IPv6 source guard: Default ace rule could not be deleted.";

    case IPV6_SOURCE_GUARD_ERROR_BINDING_TABLE_FULL:
        return "IPv6 source guard: No entries can currently be added, binding table is full.";

    case IPV6_SOURCE_GUARD_ERROR_DYNAMIC_MAX_REACHED:
        return "IPv6 source guard: Port has reached max dynamic entries limit.";

    case IPV6_SOURCE_GUARD_ERROR_DISABLED:
        return "IPv6 source guard: Module/Port is disabled.";

    case IPV6_SOURCE_GUARD_ERROR_ENTRY_ALREADY_IN_TABLE:
        return "IPv6 source guard: Entry already exists in table.";

    case IPV6_SOURCE_GUARD_ERROR_STATIC_LINK_LOCAL:
        return "IPv6 source guard: Link local address cannot be added as a static entry.";

    case IPV6_SOURCE_GUARD_ERROR_STATIC_UNSPEC:
        return "IPv6 source guard: Unspecified (all-zero) IPv6 address cannot be added as a static entry.";

    case IPV6_SOURCE_GUARD_ERROR_STATIC_LOOPBACK:
        return "IPv6 source guard: Loopback IPv6 address cannot be added as a static entry.";

    case IPV6_SOURCE_GUARD_ERROR_STATIC_MC:
        return "IPv6 source guard: Multicast IPv6 address cannot be added as a static entry.";

    default:
        return "IP source guard: unknown error occurred.";
    }
}

/*************************************************************************
** ACE + QCE Related Functions
*************************************************************************/

/*
 * Add ACE rule for binding entries, is base for policy.
 */
static mesa_rc ipv6_source_guard_base_binding_ace_add() 
{
    acl_entry_conf_t acl_conf;

    T_D("enter");

    /* Initialize ace rule. */
    if (acl_mgmt_ace_init(MESA_ACE_TYPE_ANY, &acl_conf) != MESA_RC_OK) {
        T_W("Could no init ace rule, exit");
        return IPV6_SOURCE_GUARD_ERROR_DEFAULT_ACE_RULE;
    }

    /* Adding rule components. */
    acl_conf.isid = VTSS_ISID_LOCAL;
    acl_conf.id = ACE_ID_BINDING_BASE;
    acl_conf.port_list.set_all();
    acl_conf.policy.value = ACL_POLICY_IPV6_SOURCE_GUARD_ENTRY;
    acl_conf.policy.mask = 0xFF;
    acl_conf.action.port_action = MESA_ACL_PORT_ACTION_NONE;
    acl_conf.action.port_list.set_all();

    if (acl_mgmt_ace_add(ACL_USER_IPV6_SOURCE_GUARD, ACE_ID_PORT_DEFAULT, &acl_conf) != MESA_RC_OK) {
        T_W("Could not add ace rule, exit");
        return IPV6_SOURCE_GUARD_ERROR_DEFAULT_ACE_RULE;
    }

    T_D("exit");
    return VTSS_RC_OK;  
}

/* 
 * Add binding entry QCE rule 
 */
static mesa_rc ipv6_source_guard_qce_binding_entry_add(
    vtss_appl_ipv6_source_guard_entry_index_t entry, 
    vtss_appl_ipv6_source_guard_entry_qce_data_t *entry_data,
    mesa_port_no_t port_no) 
{
    vtss_appl_qos_qce_conf_t conf;
    mesa_rc rc;

    T_D("enter");

    if ((rc = vtss_appl_qos_qce_conf_get_default(&conf)) != MESA_RC_OK) {
        T_D("Could not init qce rule, exit");
        return rc;
    }

    conf.qce_id = get_free_qce_id();

    if (conf.qce_id == VTSS_APPL_QOS_QCE_ID_NONE) {
        T_W("Cannot add new qce rule - no free QCE IDs, exit");
        return IPV6_SOURCE_GUARD_ERROR_BINDING_TABLE_FULL;
    }

    conf.user_id = VTSS_APPL_QOS_QCL_USER_IPV6_SOURCE_GUARD;
    conf.key.type = VTSS_APPL_QOS_QCE_TYPE_IPV6;

    /* Set rule on the relevant port. */
    memset(conf.key.port_list.data, 0x00, sizeof(conf.key.port_list.data));
    mgmt_types_port_list_bit_value_set(&conf.key.port_list, VTSS_ISID_START, port_no);

    /* Setting rule vlan id. If id is 0, no vlan id was added by user. */
    if (entry.vlan_id != 0) {
        conf.key.tag.vid.match = VTSS_APPL_VCAP_ASR_TYPE_SPECIFIC;
        conf.key.tag.vid.low = entry.vlan_id;
        conf.key.tag.vid.high = entry.vlan_id;
    }

    /* Setting rule source ipv6 address and mask */
    conf.key.frame.ipv6.sip.value = entry.ipv6_addr;
    memset(&(conf.key.frame.ipv6.sip.mask), 0xFF, sizeof(conf.key.frame.ipv6.sip.mask));

    /* Setting rule source mac and mask */
    conf.key.mac.smac.value = entry_data->mac_addr;
    memset(&(conf.key.mac.smac.mask), 0xFF, sizeof(conf.key.mac.smac.mask));

    // Set a policy value to hit the associated ACL rule
    conf.action.policy_no_enable = true;
    conf.action.policy_no = ACL_POLICY_IPV6_SOURCE_GUARD_ENTRY;

    /* Add the rule */
    if ((rc = vtss_appl_qos_qce_conf_add(conf.qce_id, &conf)) == MESA_RC_OK) {
        entry_data->qce_id = conf.qce_id;
        IPV6_SOURCE_GUARD_CRIT_ENTER();
        global_data.qce_id_used_list.push_back(conf.qce_id);
        IPV6_SOURCE_GUARD_CRIT_EXIT();
    } else {
        release_qce_id(conf.qce_id);
    }

    T_D("exit");
    return rc;
}

/* 
 * Delete binding entry QCE rule 
 */
static mesa_rc ipv6_source_guard_qce_binding_entry_delete(mesa_qce_id_t qce_id) 
{
    mesa_rc rc;
    
    T_D("enter");

    rc = vtss_appl_qos_qce_intern_del(VTSS_ISID_GLOBAL, VTSS_APPL_QOS_QCL_USER_IPV6_SOURCE_GUARD, qce_id);

    if (rc == VTSS_RC_OK) {
        release_qce_id(qce_id);
    }

    T_D("exit");
    return rc;
}

/*
 * Add Port QCE rule that allows link local traffic (fe80::/10) through source guard
 */
static mesa_rc ipv6_source_guard_allow_link_local_qce_add(vtss_ifindex_t ifindex) 
{
    vtss_appl_qos_qce_conf_t conf;
    mesa_rc rc;
    mesa_ipv6_t address;
    mesa_port_no_t port_no;
    vtss_appl_ipv6_source_guard_port_config_info_t port_conf;

    T_D("enter");

    if (ipv6_source_guard_port_configuration.get(ifindex, &port_conf) != MESA_RC_OK) {
        T_D("Could not get port configuration. Exit");
        return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
    }

    if (port_conf.link_local_qce_id != VTSS_APPL_QOS_QCE_ID_NONE) {
        T_D("Port already has a link local qce rule, exit");
        return VTSS_RC_OK;
    }

    port_no = get_port_from_ifindex(ifindex);

    if ((rc = vtss_appl_qos_qce_conf_get_default(&conf)) != MESA_RC_OK) {
        T_D("Could not init qce rule, exit");
        return rc;
    }

    conf.qce_id = get_free_qce_id();

    if (conf.qce_id == VTSS_APPL_QOS_QCE_ID_NONE) {
        T_W("Cannot add new qce rule - no free QCE IDs, exit");
        return IPV6_SOURCE_GUARD_ERROR_BINDING_TABLE_FULL;
    }
    
    conf.user_id = VTSS_APPL_QOS_QCL_USER_IPV6_SOURCE_GUARD;
    conf.key.type = VTSS_APPL_QOS_QCE_TYPE_IPV6;

    /* Set rule on the relevant port. */
    memset(conf.key.port_list.data, 0x00, sizeof(conf.key.port_list.data));
    mgmt_types_port_list_bit_value_set(&conf.key.port_list, VTSS_ISID_START, port_no);
    
    /* Set link local address and mask.*/
    memset(&(address), 0x00, sizeof(address));
    address.addr[0] = 0xfe;
    address.addr[1] = 0x80;
    conf.key.frame.ipv6.sip.value = address;
    
    memset(&(conf.key.frame.ipv6.sip.mask), 0x00, sizeof(conf.key.frame.ipv6.sip.mask));
    conf.key.frame.ipv6.sip.mask.addr[0] = 0xff;
    conf.key.frame.ipv6.sip.mask.addr[1] = 0xc0;

    // Set a policy value to hit the associated ACL rule
    conf.action.policy_no_enable = true;
    conf.action.policy_no = ACL_POLICY_IPV6_SOURCE_GUARD_ENTRY;

    /* Add the rule */
    if ((rc = vtss_appl_qos_qce_conf_add(conf.qce_id, &conf)) == MESA_RC_OK) {
        
        /* Update port configuartion with qce_id.*/
        port_conf.link_local_qce_id = conf.qce_id; 
        if (ipv6_source_guard_port_configuration.set(ifindex, &port_conf) != MESA_RC_OK) {
            T_D("Could not update port configuration. Exit");
            return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
        }

        IPV6_SOURCE_GUARD_CRIT_ENTER();
        global_data.qce_id_used_list.push_back(conf.qce_id);
        IPV6_SOURCE_GUARD_CRIT_EXIT();

    } else {
        release_qce_id(conf.qce_id);
    }

    T_D("exit");
    return rc;
}

/*
 * Delete Port QCE rule that allows link local traffic (fe80::/10) through source guard
 */
static mesa_rc ipv6_source_guard_allow_link_local_qce_delete(vtss_ifindex_t ifindex) 
{
    vtss_appl_ipv6_source_guard_port_config_info_t port_conf;

    T_D("enter");

    if (ipv6_source_guard_port_configuration.get(ifindex, &port_conf) != MESA_RC_OK) {
        T_D("Could not get port configuration. Exit");
        return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
    }

    if (port_conf.link_local_qce_id == VTSS_APPL_QOS_QCE_ID_NONE) {
        T_D("No link-local port rule present, exit");
        return VTSS_RC_OK;
    }

    if (ipv6_source_guard_qce_binding_entry_delete(port_conf.link_local_qce_id) != VTSS_RC_OK) {
        T_D("Could not delete link local qce rule, exit");
        return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
    }

    port_conf.link_local_qce_id = VTSS_APPL_QOS_QCE_ID_NONE;
    port_conf.curr_dynamic_entries = 0;

    if (ipv6_source_guard_port_configuration.set(ifindex, &port_conf) != MESA_RC_OK) {
        T_D("Could not update port configuration. Exit");
        return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
    }

    T_D("exit");
    return VTSS_RC_OK;

}

/* 
 * Add default ACE rule to port 
 */
static mesa_rc ipv6_source_guard_default_ace_add(mesa_port_no_t port_no) 
{
    acl_entry_conf_t conf;

    T_D("enter");

    /* Check if default port rule has already been initialized. */
    if (acl_mgmt_ace_get(ACL_USER_IPV6_SOURCE_GUARD, ACE_ID_PORT_DEFAULT, &conf, NULL, FALSE) == VTSS_RC_OK) {
        conf.port_list[port_no] = TRUE;
        T_D("exit");
        return acl_mgmt_ace_add(ACL_USER_IPV6_SOURCE_GUARD, ACL_MGMT_ACE_ID_NONE, &conf);
    }

    /* Initialize default ace rule. */
    if (acl_mgmt_ace_init(MESA_ACE_TYPE_IPV6, &conf) != MESA_RC_OK) {
        T_D("Could no init ace rule, exit");
        return IPV6_SOURCE_GUARD_ERROR_DEFAULT_ACE_RULE;
    }

    /* Set up components of default rule. */
    conf.isid = VTSS_ISID_LOCAL;
    conf.id = ACL_MGMT_ACE_ID_NONE;
    conf.action.port_action = MESA_ACL_PORT_ACTION_FILTER;
    memset(conf.action.port_list, 0, sizeof(conf.action.port_list));
    memset(conf.port_list, 0, sizeof(conf.port_list));
    conf.port_list[port_no] = TRUE;

    /* Add the rule */
    if (acl_mgmt_ace_add(ACL_USER_IPV6_SOURCE_GUARD, ACL_MGMT_ACE_ID_NONE, &conf) != MESA_RC_OK) {
        T_W("Could no add ace rule, exit");
        return IPV6_SOURCE_GUARD_ERROR_DEFAULT_ACE_RULE;
    }

    IPV6_SOURCE_GUARD_CRIT_ENTER();
    global_data.ace_port_id_default = conf.id;
    IPV6_SOURCE_GUARD_CRIT_EXIT();

    /* Adding the rule to which the qce rules will be bound through policy. */
    if (ipv6_source_guard_base_binding_ace_add() != MESA_RC_OK) {
        T_W("Could not add ace rule, exit");
        return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
    }
    
    T_D("exit");
    return VTSS_RC_OK;
}

/* 
 * Delete default ACE rule from port 
 */
static mesa_rc ipv6_source_guard_default_ace_delete(mesa_port_no_t del_port_no) 
{
    mesa_qce_id_t curr_ace_id;
    acl_entry_conf_t conf;

    T_D("enter");

    if (acl_mgmt_ace_get(ACL_USER_IPV6_SOURCE_GUARD, 1, &conf, NULL, FALSE) == VTSS_RC_OK) {
        conf.port_list[del_port_no] = FALSE;

        for (mesa_port_no_t port_no_itr = 0; port_no_itr < port_count_max(); port_no_itr++) {
            
            /* If another port is still enabled, the rule is updated without the del_port*/
            if (conf.port_list[port_no_itr] == TRUE) {
                T_D("exit");
                return acl_mgmt_ace_add(ACL_USER_IPV6_SOURCE_GUARD, ACL_MGMT_ACE_ID_NONE, &conf);
            }
        }

        /* If no other ports are enabled, the rule is deleted. */
        IPV6_SOURCE_GUARD_CRIT_ENTER();
        curr_ace_id = global_data.ace_port_id_default;
        global_data.ace_port_id_default = ACL_MGMT_ACE_ID_NONE;
        IPV6_SOURCE_GUARD_CRIT_EXIT();

        /* First delete base binding rule. */
        if (acl_mgmt_ace_del(ACL_USER_IPV6_SOURCE_GUARD, ACE_ID_BINDING_BASE) != VTSS_RC_OK) {
            T_W("Not able to delete base binding ace rule");
        }

        T_D("exit");
        return acl_mgmt_ace_del(ACL_USER_IPV6_SOURCE_GUARD, curr_ace_id);
    }

    T_D("exit");
    return VTSS_RC_OK;
}

/* 
 * Clear all QCEs and ACEs of IPv6 source guard 
 */
static void ipv6_source_guard_ace_qce_clear()
{
    acl_entry_conf_t ace_conf;
    mesa_rc rc;

    IPV6_SOURCE_GUARD_CRIT_ENTER();
    for (auto it = global_data.qce_id_used_list.begin(); it != global_data.qce_id_used_list.end(); it++) {
        if ((rc = vtss_appl_qos_qce_intern_del(VTSS_ISID_GLOBAL, VTSS_APPL_QOS_QCL_USER_IPV6_SOURCE_GUARD, *it)) != VTSS_RC_OK) {
            T_D("Could not delete qce rule, %s", vtss_appl_qos_error_txt(rc));
        } else {
            release_qce_id(*it);
        }
    }

    global_data.qce_id_used_list.clear();
    IPV6_SOURCE_GUARD_CRIT_EXIT();

    while (acl_mgmt_ace_get(ACL_USER_IPV6_SOURCE_GUARD, ACL_MGMT_ACE_ID_NONE, &ace_conf, NULL, 1) == VTSS_RC_OK) {
        if (acl_mgmt_ace_del(ACL_USER_IPV6_SOURCE_GUARD, ace_conf.id)) {
            T_D("acl_mgmt_ace_del(ACL_USER_IPV6_SOURCE_GUARD, %d) failed", ace_conf.id);
        }
    }
}

/*************************************************************************
 ** Register functions                                                      
 *************************************************************************/

static void ipv6_source_guard_dhcp6_snoop_entry_receive(
    const vtss_appl_dhcp6_snooping_client_info_t *client_info,
    const vtss_appl_dhcp6_snooping_assigned_ip_t *address_info,
    vtss_appl_dhcp6_snooping_info_reason_t reason)
{
    vtss_appl_ipv6_source_guard_entry_index_t index;
    vtss_appl_ipv6_source_guard_entry_qce_data_t data;

    T_D("enter");

    memset(&index, 0x0, sizeof(index));
    memset(&data, 0x0, sizeof(data));
    
    data.mac_addr = client_info->mac;
    //memcpy(data.mac_addr.addr, client_info->mac.addr, sizeof(data.mac_addr.addr));
    data.qce_id = VTSS_APPL_QOS_QCE_ID_NONE;

    index.ifindex = client_info->if_index;
    index.ipv6_addr = address_info->ip_address;
    index.vlan_id = address_info->vid;

    if (reason == DHCP6_SNOOPING_INFO_REASON_ASSIGNED) {
        if (ipv6_source_guard_dynamic_entry_set(index, &data) != VTSS_RC_OK) {
            T_D("ipv6_source_guard_dynamic_entry_set() failed, exit");
        }
    } else {
        if (ipv6_source_guard_dynamic_entry_del(index) != VTSS_RC_OK) {
            T_D("ipv6_source_guard_dynamic_entry_del() failed, exit");
        }
    }

    T_D("exit");
}

/*************************************************************************
** Misc. Static Helper Functions
**************************************************************************/

/* 
 * Gets entry info from snooping module and adds to source guard if applicable.
 */
static mesa_rc ipv6_source_guard_get_snooping_entries(
    mesa_port_no_t port_no, mesa_bool_t check_for_entry, const vtss_appl_ipv6_source_guard_entry_index_t *entry_index)
{
    mesa_port_no_t curr_port_no;
    vtss_appl_dhcp6_snooping_duid_t *prev_duid;
    vtss_appl_dhcp6_snooping_duid_t next_duid;
    vtss_appl_dhcp6_snooping_iaid_t *prev_iaid;
    vtss_appl_dhcp6_snooping_iaid_t next_iaid;
    vtss_appl_dhcp6_snooping_assigned_ip_t address_info;
    vtss_appl_dhcp6_snooping_client_info_t client_info;
    vtss_appl_ipv6_source_guard_entry_index_t index;
    vtss_appl_ipv6_source_guard_entry_index_t tmp_index;
    vtss_appl_ipv6_source_guard_entry_qce_data_t data;

    T_D("enter");

    prev_duid = NULL;
    prev_iaid = NULL;

    while (vtss_appl_dhcp6_snooping_assigned_ip_itr(
            prev_duid, &next_duid, prev_iaid, &next_iaid) == VTSS_RC_OK) {
        
        if (vtss_appl_dhcp6_snooping_client_info_get(next_duid, &client_info) != VTSS_RC_OK) {
            T_D("Could not get client info from snooping module, exit");
            return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
        }
        if (vtss_appl_dhcp6_snooping_assigned_ip_get(next_duid, next_iaid, &address_info) != VTSS_RC_OK) {
            T_D("Could not get address info from snooping module, exit");
            return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
        }

        /* Check if the current entry should be added. */
        curr_port_no = get_port_from_ifindex(client_info.if_index);
        if (curr_port_no != port_no) {
            prev_duid = &next_duid;
            prev_iaid = &next_iaid;
            T_D("Not the correct port.");
            continue;
        }

        /* A check in case we are searching for one, particular entry.*/
        if (check_for_entry) {
            tmp_index.ifindex = client_info.if_index;
            tmp_index.ipv6_addr = address_info.ip_address;
            tmp_index.vlan_id = address_info.vid;
            if (*entry_index != tmp_index) {
                prev_duid = &next_duid;
                prev_iaid = &next_iaid;
                T_D("Not the correct entry.");
                continue;
            }
        }

        data.mac_addr = client_info.mac;
        data.qce_id = VTSS_APPL_QOS_QCE_ID_NONE;

        index.ifindex = client_info.if_index;
        index.ipv6_addr = address_info.ip_address;
        index.vlan_id = address_info.vid;

        if (ipv6_source_guard_dynamic_entry_set(index, &data) != VTSS_RC_OK) {
            T_D("ipv6_source_guard_dynamic_entry_set() failed");
        }

        /* If only searching for a specific entry, now it has been added and we can return. */
        if (check_for_entry) {
            break;
        }

        prev_duid = &next_duid;
        prev_iaid = &next_iaid;
    }

    T_D("exit");
    return VTSS_RC_OK;
}

/*
 * Deletes dynamic entry at index. 
 */
static mesa_rc ipv6_source_guard_curr_dynamic_entry_del(
    vtss_appl_ipv6_source_guard_entry_index_t index, mesa_qce_id_t qce_id) 
{
    
        /* Deleting qce rule and entry from dynamic table.*/
        if (ipv6_source_guard_qce_binding_entry_delete(qce_id) != VTSS_RC_OK) {
            T_W("Could not delete qce rule, exit");
            return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
        }

        if (ipv6_source_guard_dynamic_entries.del(index) != VTSS_RC_OK) {
             T_W("Could not delete entry from dynamic table, exit");
            return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
        }
    return VTSS_RC_OK;
}

/* 
 * Iterates dynamic table and deletes entries for current port. 
 */
static mesa_rc ipv6_source_guard_remove_dynamic_port_entries(mesa_port_no_t port_no, uint32_t num_to_skip) 
{
    vtss_appl_ipv6_source_guard_entry_index_t *prev = NULL;
    vtss_appl_ipv6_source_guard_entry_index_t next;
    vtss_appl_ipv6_source_guard_entry_qce_data_t data;
    mesa_port_no_t curr_port_no;
    uint32_t entries_skipped = 0;

    while (ipv6_source_guard_dynamic_entry_itr(prev, &next) == VTSS_RC_OK) {
        
        if (ipv6_source_guard_dynamic_entries.get(next, &data) != VTSS_RC_OK) {
            T_W("Could not get dynamic entry, exit");
            return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
        }

        curr_port_no = get_port_from_ifindex(next.ifindex);

        if (curr_port_no != port_no) {
            prev = &next;
            continue;
        }

        if (entries_skipped < num_to_skip) {
            entries_skipped++;
            prev = &next;
            continue;
        }

        /* Deleting qce rule and entry from dynamic table.*/
        if (ipv6_source_guard_curr_dynamic_entry_del(next, data.qce_id) != VTSS_RC_OK) {
            T_W("Could not delete dynamic entry, exit");
            return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
        }
        prev = &next;
    }

    T_D("Entries skipped %u, entries to skip %u\n", entries_skipped, num_to_skip);
    return VTSS_RC_OK;
}

/*
 * Iterates static entry table and adds qce rules for current port.
 */
static mesa_rc ipv6_source_guard_add_static_port_entries(mesa_port_no_t port_no) 
{
    vtss_appl_ipv6_source_guard_entry_index_t *prev = NULL;
    vtss_appl_ipv6_source_guard_entry_index_t next;
    vtss_appl_ipv6_source_guard_entry_qce_data_t data;
    mesa_port_no_t curr_port_no;

    T_D("enter");

    while (ipv6_source_guard_static_entry_itr(prev, &next) == VTSS_RC_OK) {

        if (ipv6_source_guard_static_entries.get(next, &data) != VTSS_RC_OK) {
            T_W("Could not get static entry, exit");
            return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
        }

        curr_port_no = get_port_from_ifindex(next.ifindex);
        
        if (curr_port_no != port_no) {
            prev = &next;
            continue;
        }

        if (ipv6_source_guard_qce_binding_entry_add(next, &data, port_no) != VTSS_RC_OK) {
            T_W("Could not add entry qce rule, exit");
            return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
        }

        /* Update entry with the new qce id. */
        if (ipv6_source_guard_static_entries.set(next, &data) != VTSS_RC_OK) {
            T_W("Could not update static entry, exit");
            return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
        }
        
        prev = &next;
    }

    T_D("exit");
    return VTSS_RC_OK;
}

/*
 * Iterates static entry table and deletes qce rules for current port.
 */
static mesa_rc ipv6_source_guard_remove_static_port_entries(mesa_port_no_t port_no) 
{
    vtss_appl_ipv6_source_guard_entry_index_t *prev = NULL;
    vtss_appl_ipv6_source_guard_entry_index_t next;
    vtss_appl_ipv6_source_guard_entry_qce_data_t data;
    mesa_port_no_t curr_port_no;

    /* Static entries. */
    while (ipv6_source_guard_static_entry_itr(prev, &next) == VTSS_RC_OK) {

        if (ipv6_source_guard_static_entries.get(next, &data) != VTSS_RC_OK) {
            T_W("Could not get static entry, exit");
            return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
        }
        
        curr_port_no = get_port_from_ifindex(next.ifindex);
        
        if (curr_port_no != port_no)  {
            prev = &next;
            continue;
        }

        if (data.qce_id == VTSS_APPL_QOS_QCE_ID_NONE) {
            prev = &next;
            continue;
        }

        /* Deleting qce rule and updating entry in static table.*/
        if (ipv6_source_guard_qce_binding_entry_delete(data.qce_id) != VTSS_RC_OK) {
            T_W("Could not delete qce rule, exit");
            return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
        }

        data.qce_id = VTSS_APPL_QOS_QCE_ID_NONE;

        if (ipv6_source_guard_static_entries.set(next, &data) != VTSS_RC_OK) {
            T_W("Could not update binding entry, exit");
            return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
        }
        prev = &next;
    }
    
    return VTSS_RC_OK;
}

/*
 * Sets port_conf to default values.
 */
static void ipv6_source_guard_get_port_default_conf(
    vtss_appl_ipv6_source_guard_port_config_info_t *port_conf) 
{   
    port_conf->conf.enabled = false;
    port_conf->conf.max_dynamic_entries = IPV6_SOURCE_GUARD_DYNAMIC_UNLIMITED;
    port_conf->curr_dynamic_entries = 0;
    port_conf->link_local_qce_id = VTSS_APPL_QOS_QCE_ID_NONE;
} 

/* 
 * Set default port configuration 
 */
static mesa_rc ipv6_source_guard_set_port_default_conf() 
{
    vtss_ifindex_t ifindex;
    vtss_appl_ipv6_source_guard_port_config_info_t default_port_conf;

    T_D("enter");

    ipv6_source_guard_get_port_default_conf(&default_port_conf);

    /* Initialize all ports to disabled state. */
    for (mesa_port_no_t port_no = 0; port_no < port_count_max(); port_no++) {
        if (vtss_ifindex_from_port(VTSS_ISID_START,port_no, &ifindex) != VTSS_RC_OK) {
            T_W("Ifindex could not be derived from port_no %u", port_no);
        }
        if (ipv6_source_guard_port_configuration.set(ifindex, &default_port_conf) != VTSS_RC_OK) {
            T_W("Default configuration could not be set for port %u", port_no);
        }
    }

    T_D("exit");
    return VTSS_RC_OK;
}

/*
 * Delete additional dynamic entries if port limit has been reduced. 
 */
static mesa_rc ipv6_source_guard_extra_dynamic_entries_del(vtss_ifindex_t ifindex, uint32_t limit, uint32_t curr_number)
{
    uint32_t num_entries_to_delete = 0;
    mesa_port_no_t port_no = get_port_from_ifindex(ifindex);
    
    if (limit == 0) {
        return ipv6_source_guard_remove_dynamic_port_entries(port_no, 0);
    }

    num_entries_to_delete = curr_number - limit;
    T_D("Deleting %u dynamic entries\n", num_entries_to_delete);
    return ipv6_source_guard_remove_dynamic_port_entries(port_no, limit);
} 

/* 
 * Enabling static entries, getting dynamic data from dhcpv6 snooping and setting ace + qce rules
 */
static mesa_rc ipv6_source_guard_port_enable(vtss_ifindex_t ifindex) 
{
    mesa_port_no_t port_no;

    T_D("enter");

    port_no = get_port_from_ifindex(ifindex);

    /* Adding default ace rule. */
    if (ipv6_source_guard_default_ace_add(port_no) != VTSS_RC_OK) {
        T_W("Could not add default port ace rule, exit");
        return IPV6_SOURCE_GUARD_ERROR_DEFAULT_ACE_RULE;
    }

    /* Add rule that allows link local addresses. */
    if (ipv6_source_guard_allow_link_local_qce_add(ifindex) != MESA_RC_OK) {
        T_W("Could not add link local qce rule");
    }

    /* Adding qce rules for port static entries. */
    if (ipv6_source_guard_add_static_port_entries(port_no) != VTSS_RC_OK) {
        T_W("Could not add qce rules for static entries, exit");
        return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
    }

    /* Gettin snooping data and adding dynamic entries. */
    if (ipv6_source_guard_get_snooping_entries(port_no, false, NULL) != VTSS_RC_OK) {
        T_D("Could not add dynamic entries for port, exit");
        return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
    }

    T_D("exit");
    return VTSS_RC_OK;
}

/* 
 * Disabling/deleting binding port entries and deleting port ace and qce rules 
 */
static mesa_rc ipv6_source_guard_port_disable(vtss_ifindex_t ifindex) 
{
    mesa_port_no_t port_no;
    
    T_D("enter");
    
    port_no = get_port_from_ifindex(ifindex);

    /* Static entries. */
    if (ipv6_source_guard_remove_static_port_entries(port_no) != VTSS_RC_OK) {
        T_D("Could not remove static port entries.");
        return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
    } 

    /* Dynamic entries. */
    if (ipv6_source_guard_remove_dynamic_port_entries(port_no, 0) != VTSS_RC_OK) {
        T_D("Could not remove static port entries.");
        return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
    } 

    /* Allow link local addresses qce rule. */
    if (ipv6_source_guard_allow_link_local_qce_delete(ifindex) != VTSS_RC_OK) {
        T_D("Could not remove port link local qce rule");
        return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
    }
   
    /* Default ace rule. */
    if (ipv6_source_guard_default_ace_delete(port_no) != VTSS_RC_OK) {
        T_W("Could not delete default port ace rule, exit");
        return IPV6_SOURCE_GUARD_ERROR_DEFAULT_ACE_RULE_DEL;
    }

    T_D("exit");
    return VTSS_RC_OK;
}

/* 
 * Setting module default configuration 
 */
static mesa_rc ipv6_source_guard_set_default_conf() 
{
    T_D("enter");

    IPV6_SOURCE_GUARD_CRIT_ENTER();
    default_conf.enabled = false;
    global_data.ace_port_id_default = ACL_MGMT_ACE_ID_NONE;
    global_data.qce_id_free_list.clear();
    global_data.qce_id_used_list.clear();

    for (uint32_t count = 0; count < IPV6_SOURCE_GUARD_MAX_ENTRY_CNT + port_count_max(); count++) {
        mesa_qce_id_t qce_id = QCE_ID_SESSION_START + count;
        global_data.qce_id_free_list.push_back(qce_id);
    }

    IPV6_SOURCE_GUARD_CRIT_EXIT();

    /* Clearing tables */
    ipv6_source_guard_port_configuration.clear();
    ipv6_source_guard_static_entries.clear();
    ipv6_source_guard_dynamic_entries.clear();

    /* Setting default port configuration */
    if (ipv6_source_guard_set_port_default_conf() != VTSS_RC_OK) {
        T_W("Ports could not be configured. Exit.");
        return IPV6_SOURCE_GUARD_ERROR_PORT_CONFIGURE;
    }

    T_D("exit");
    return VTSS_RC_OK;
}

/* 
 * Reloading module default configuration 
 */
static mesa_rc ipv6_source_guard_reload_default_conf() 
{
    mesa_rc rc = VTSS_RC_OK;
    T_D("enter");

    /* Clearing all IPv6 source guard ace rules.*/
    ipv6_source_guard_ace_qce_clear();

    /* Setting default configuration. */
    if ((rc = ipv6_source_guard_set_default_conf()) != VTSS_RC_OK) {
        T_W("Default configuration could not be set");
    }

    T_D("exit");
    return rc;

}

/* 
 * Enable ipv6 source guard globally 
 */
static mesa_rc ipv6_source_guard_global_enable()
{
    vtss_ifindex_t *prev = NULL;
    vtss_ifindex_t next;
    vtss_appl_ipv6_source_guard_port_config_t port_conf;

    T_D("enter");

    while (ipv6_source_guard_port_conf_itr(prev, &next) == VTSS_RC_OK) {
        ipv6_source_guard_port_config_get(next, &port_conf);
        if (port_conf.enabled) {
            /* Set up qce rules for port. */
            if (ipv6_source_guard_port_enable(next) != VTSS_RC_OK) {
                T_W("Could not enableport, exit");
                return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
            }
        }
        prev = &next;
    }
    
    if (vtss_appl_dhcp6_snooping_assigned_ip_info_register(ipv6_source_guard_dhcp6_snoop_entry_receive) != VTSS_RC_OK) {
        T_D("Could not register to snooping module");
    }
    
    T_D("exit");
    return VTSS_RC_OK;
}

/* 
 * Disable ipv6 source guard globally 
 */
static mesa_rc ipv6_source_guard_global_disable()
{
    vtss_ifindex_t *prev = NULL;
    vtss_ifindex_t next;
    vtss_appl_ipv6_source_guard_port_config_t port_conf;

    T_D("enter");

    while (ipv6_source_guard_port_conf_itr(prev, &next) == VTSS_RC_OK) {
        ipv6_source_guard_port_config_get(next, &port_conf);
        if (port_conf.enabled) {
            /* Delete binding qce rules, default ace rule and deleting/disabling binding entries */
            if (ipv6_source_guard_port_disable(next) != VTSS_RC_OK) {
                T_W("Could not delete port ace/qce rules, exit");
                return IPV6_SOURCE_GUARD_ERROR_DEFAULT_ACE_RULE_DEL;
            }
        }
        prev = &next;
    }

    if (vtss_appl_dhcp6_snooping_assigned_ip_info_unregister(ipv6_source_guard_dhcp6_snoop_entry_receive) != VTSS_RC_OK) {
        T_D("Could not unregister from snooping module");
    }

    ipv6_source_guard_dynamic_entries.clear();

    T_D("exit");
    return VTSS_RC_OK;
}

/* 
 * Translate dynamic binding entry to a static entry
 */
static mesa_rc ipv6_source_guard_translate_dynamic_to_static(
    vtss_appl_ipv6_source_guard_entry_index_t index)
{
    vtss_appl_ipv6_source_guard_entry_qce_data_t data;
    vtss_appl_ipv6_source_guard_port_config_info_t port_data;
    mesa_rc rc;

    T_D("enter");

    if ((rc = ipv6_source_guard_dynamic_entries.get(index, &data)) == MESA_RC_OK) {

        if ((rc = ipv6_source_guard_static_entries.set(index, &data)) != MESA_RC_OK) {
            T_D("Could not add entry to static table, exit");
            return rc;
        }

        if ((rc = ipv6_source_guard_dynamic_entries.del(index)) != MESA_RC_OK) {
            T_D("Could not delete entry from dynamic table, exit");
            return rc;
        }

        if ((rc = ipv6_source_guard_port_configuration.get(index.ifindex, &port_data)) != MESA_RC_OK) {
            T_D("Could not get current port data");
            return rc;
        }

        port_data.curr_dynamic_entries--;

        if ((rc = ipv6_source_guard_port_configuration.set(index.ifindex, &port_data)) != MESA_RC_OK) {
            T_D("Could not update port data with new current dynamic entry count");
            return rc;
        }
    }

    T_D("exit");
    return rc;
}

/*
 * Translate all dynamic entries to static.
 */
static mesa_rc ipv6_source_guard_translate_all_dynamic_entries()
{
    vtss_appl_ipv6_source_guard_entry_index_t *prev = NULL;
    vtss_appl_ipv6_source_guard_entry_index_t next;

    T_D("enter");

    while (ipv6_source_guard_dynamic_entry_itr(prev, &next) == VTSS_RC_OK) {

        /* Translating dynamic entry.*/
        if (ipv6_source_guard_translate_dynamic_to_static(next) != VTSS_RC_OK) {
            T_W("Could not translate dynamic entry, exit");
            return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
        }

        prev = &next;
    }

    T_D("exit");
    return VTSS_RC_OK;
}

/*
 * Add dynamic entry to module.
 */
static mesa_rc ipv6_source_guard_dynamic_entry_set(
    vtss_appl_ipv6_source_guard_entry_index_t index,
    vtss_appl_ipv6_source_guard_entry_qce_data_t *data)
{
    mesa_rc rc;
    mesa_port_no_t port_no;
    vtss_appl_ipv6_source_guard_entry_qce_data_t temp_data;
    vtss_appl_ipv6_source_guard_port_config_info_t port_conf;
    
    T_D("enter");

    /* Check if module and port are enabled. */
    if ((rc = ipv6_source_guard_global_port_enabled_check(index.ifindex)) != MESA_RC_OK) {
        T_D("Port/Module not enabled, exit");
        return VTSS_RC_OK;
    }

    /* Check if total number of binding entries has reached max allowed. */
    if (get_total_number_of_entries() >= IPV6_SOURCE_GUARD_MAX_ENTRY_CNT) {
        T_D("Ipv6 source guard: entry table full, exit");
        return IPV6_SOURCE_GUARD_ERROR_BINDING_TABLE_FULL;
    }

    /* Check if number of dynamic entries has reached max allowed on port. */
    if (ipv6_source_guard_port_configuration.get(index.ifindex, &port_conf) != MESA_RC_OK) {
        T_D("Could not get port configuration, exit");
        return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
    }

    if (port_conf.curr_dynamic_entries >= port_conf.conf.max_dynamic_entries) {
        T_D("Port has reached maximum dynamic entries allowed. Entry will not be added, exit");
        T_D("Curr_entries: %u, max_entries: %u\n", port_conf.curr_dynamic_entries, port_conf.conf.max_dynamic_entries);
        return IPV6_SOURCE_GUARD_ERROR_DYNAMIC_MAX_REACHED;
    }

    /* Check if entry already exists in dynamic table. */
    if (ipv6_source_guard_dynamic_entries.get(index, &temp_data) == MESA_RC_OK) {
        T_D("Entry already exists in dynamic table, exit");
        return VTSS_RC_OK;
    }

    /* Check if entry already exists in static table. */
    if (ipv6_source_guard_static_entries.get(index, &temp_data) == MESA_RC_OK) {
        T_D("Entry already exists in static table, exit");
        return VTSS_RC_OK;
    }
    
    port_no = get_port_from_ifindex(index.ifindex);

    /* Creating new qce entry. */
    if (ipv6_source_guard_qce_binding_entry_add(index, data, port_no) != MESA_RC_OK) {
        T_D("Could not create qce rule for entry, exit");
        return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
    }
    
    port_conf.curr_dynamic_entries++;

    /* Update port conf with new curr_dynamic_entries count. */
    if (ipv6_source_guard_port_configuration.set(index.ifindex, &port_conf) != MESA_RC_OK) {
        T_D("Could not update port configuration, exit");
        return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
    }

    /* Add entry to dynamic table*/
    if (ipv6_source_guard_dynamic_entries.set(index, data) != MESA_RC_OK) {
        T_D("Could not add entry to dynamic table, exit");
        return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
    }

    T_D("exit");
    return VTSS_RC_OK;
}

/*
 * Remove dynamic entry from module. 
 */
static mesa_rc ipv6_source_guard_dynamic_entry_del(
    vtss_appl_ipv6_source_guard_entry_index_t index)
{
    mesa_rc rc;
    vtss_appl_ipv6_source_guard_entry_qce_data_t data;
    vtss_appl_ipv6_source_guard_port_config_info_t port_conf;
    
    T_D("enter");

    /* Check if module and port are enabled. */
    if ((rc = ipv6_source_guard_global_port_enabled_check(index.ifindex)) != MESA_RC_OK) {
        T_D("Port/Module not enabled, exit");
        return VTSS_RC_OK;
    }

    if (ipv6_source_guard_dynamic_entries.get(index, &data) != MESA_RC_OK) {
        T_D("Entry did not exists in dynamic table, exit");
        return VTSS_RC_OK;
    }

    /* Check if there is an QCE rule for this entry. */
    if (data.qce_id != VTSS_APPL_QOS_QCE_ID_NONE) {
        if (ipv6_source_guard_qce_binding_entry_delete(data.qce_id) != MESA_RC_OK) {
            T_D("ACE rule could not be deleted, exit");
            return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
        }
    }

    /* Update port curr_dynamic_entries count. */
    if (ipv6_source_guard_port_configuration.get(index.ifindex, &port_conf) != MESA_RC_OK) {
        T_D("Could not get port configuration, exit");
        return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
    }

    port_conf.curr_dynamic_entries--;

    if (ipv6_source_guard_port_configuration.set(index.ifindex, &port_conf) != MESA_RC_OK) {
        T_D("Could not update port configuration, exit");
        return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
    }

    T_D("exit");
    return ipv6_source_guard_dynamic_entries.del(index);
}

/*
 * Check if a given address is a valid address for a static entry.
 */
static mesa_rc ipv6_source_guard_check_valid_static_address(mesa_ipv6_t entry_address)
{
    if (vtss_ipv6_addr_is_link_local(&entry_address)) {
        return IPV6_SOURCE_GUARD_ERROR_STATIC_LINK_LOCAL;
    }
    if (vtss_ipv6_addr_is_zero(&entry_address)) {
        return IPV6_SOURCE_GUARD_ERROR_STATIC_UNSPEC;
    }
    if (vtss_ipv6_addr_is_loopback(&entry_address)) {
        return IPV6_SOURCE_GUARD_ERROR_STATIC_LOOPBACK;
    }
    if (vtss_ipv6_addr_is_multicast(&entry_address)) {
        return IPV6_SOURCE_GUARD_ERROR_STATIC_MC;
    }

    return VTSS_RC_OK;
}

/*
 * Helper function for checking if a certain entry needs to be added to dynamic table.
 */
static mesa_rc ipv6_source_guard_check_snooping_entries(const vtss_appl_ipv6_source_guard_entry_index_t *index) 
{
    mesa_rc rc;
    mesa_port_no_t port_no;

    port_no = get_port_from_ifindex(index->ifindex);
    rc = ipv6_source_guard_get_snooping_entries(port_no, true, index);

    return rc;
}


/*************************************************************************
** Static API Support Functions
*************************************************************************/

/* 
 * Port iterator 
 */
static mesa_rc ipv6_source_guard_port_conf_itr(
    const vtss_ifindex_t    *const prev,
    vtss_ifindex_t          *const next)
{
    vtss_appl_ipv6_source_guard_port_config_info_t port_conf;

    T_D("enter");

    if (next == nullptr) {
        T_D("exit");
        return IPV6_SOURCE_GUARD_ERROR_INV_PARAMETER;
    }

    if (prev == nullptr) {
        T_D("exit");
        return ipv6_source_guard_port_configuration.get_first(next, &port_conf);
    }

    *next = *prev;

    T_D("exit");
    return ipv6_source_guard_port_configuration.get_next(next, &port_conf);
}

/*
 * Static table iterator 
 */
static mesa_rc  ipv6_source_guard_static_entry_itr(
    const vtss_appl_ipv6_source_guard_entry_index_t  *const prev,
    vtss_appl_ipv6_source_guard_entry_index_t        *const next) 
{
    vtss_appl_ipv6_source_guard_entry_qce_data_t data;

    T_D("enter");

    if (next == nullptr) {
        T_D("exit");
        return IPV6_SOURCE_GUARD_ERROR_INV_PARAMETER;
    }

    if (prev == nullptr) {
        T_D("exit");
        return ipv6_source_guard_static_entries.get_first(next, &data);
    }

    *next = *prev;

    T_D("exit");
    return ipv6_source_guard_static_entries.get_next(next, &data);
}

/* 
 * Dynamic table iterator 
 */
static mesa_rc ipv6_source_guard_dynamic_entry_itr(
    const vtss_appl_ipv6_source_guard_entry_index_t  *const prev,
    vtss_appl_ipv6_source_guard_entry_index_t        *const next)
{
    vtss_appl_ipv6_source_guard_entry_qce_data_t data;

    T_D("enter");

    if (next == nullptr) {
        T_D("exit");
        return IPV6_SOURCE_GUARD_ERROR_INV_PARAMETER;
    }

    if (prev == nullptr) {
        T_D("exit");
        return ipv6_source_guard_dynamic_entries.get_first(next, &data);
    }

    *next = *prev;
    
    T_D("exit");
    return ipv6_source_guard_dynamic_entries.get_next(next, &data);
}

/* 
 * Get current global configuration 
 */
static mesa_rc ipv6_source_guard_global_config_get(
    vtss_appl_ipv6_source_guard_global_config_t    *const conf)
{
    T_D("enter");

    IPV6_SOURCE_GUARD_CRIT_ENTER();
    conf->enabled = default_conf.enabled;
    IPV6_SOURCE_GUARD_CRIT_EXIT();

    T_D("exit");
    return VTSS_RC_OK;
}

/* 
 * Set/change global configuration 
 */
static mesa_rc ipv6_source_guard_global_config_set(
    const vtss_appl_ipv6_source_guard_global_config_t  *const conf)
{
    bool conf_changed = false;

    T_D("enter");

    IPV6_SOURCE_GUARD_CRIT_ENTER();
    if (default_conf.enabled != conf->enabled) {
        default_conf.enabled = conf->enabled;
        conf_changed = true;
    }
    IPV6_SOURCE_GUARD_CRIT_EXIT();

    if (conf_changed) {
        if (conf->enabled) {
            T_D("exit");
            return ipv6_source_guard_global_enable();
        } else {
            T_D("exit");
            return ipv6_source_guard_global_disable();
        }
    }

    T_D("exit");
    return VTSS_RC_OK;
}

/* 
 * Get current port configuration 
 */
static mesa_rc ipv6_source_guard_port_config_get(
    vtss_ifindex_t  ifindex,
    vtss_appl_ipv6_source_guard_port_config_t  *const port_conf)
{
    mesa_rc rc;
    vtss_appl_ipv6_source_guard_port_config_info_t p_conf;

    T_D("enter");

    rc = ipv6_source_guard_port_configuration.get(ifindex, &p_conf);
    *port_conf = p_conf.conf;
    
    T_D("exit");
    return rc;
}

/* 
 * Set/change port configuration 
 */
static mesa_rc ipv6_source_guard_port_config_set(
    vtss_ifindex_t  ifindex,
    const vtss_appl_ipv6_source_guard_port_config_t   *const port_conf)
{
    vtss_appl_ipv6_source_guard_port_config_info_t curr_port_conf;
    vtss_appl_ipv6_source_guard_port_config_info_t new_port_conf;
    mesa_port_no_t port_no;
    bool mode_changed = false;
    bool port_found = false;
    bool module_globally_enabled;
    bool module_port_enabled = false;
    bool update_dynamic_entries = false;

    T_D("enter, ifindex is %zu", vtss_ifindex_cast_to_u32(ifindex));

    IPV6_SOURCE_GUARD_CRIT_ENTER();
    module_globally_enabled = default_conf.enabled;
    IPV6_SOURCE_GUARD_CRIT_EXIT();

    new_port_conf.curr_dynamic_entries = 0;

    port_found = (ipv6_source_guard_port_configuration.get(ifindex, &curr_port_conf) == VTSS_RC_OK);

    if (port_found) {
        new_port_conf.link_local_qce_id = curr_port_conf.link_local_qce_id;
        new_port_conf.curr_dynamic_entries = curr_port_conf.curr_dynamic_entries;
        module_port_enabled = module_globally_enabled && curr_port_conf.conf.enabled;
        
        /* Check if port dynamic entry limit is being updated. */
        if (curr_port_conf.conf.max_dynamic_entries != port_conf->max_dynamic_entries) {

            if ((curr_port_conf.curr_dynamic_entries > port_conf->max_dynamic_entries) && module_port_enabled) {
                if (ipv6_source_guard_extra_dynamic_entries_del(ifindex, port_conf->max_dynamic_entries, curr_port_conf.curr_dynamic_entries) == VTSS_RC_OK) {
                    new_port_conf.curr_dynamic_entries = port_conf->max_dynamic_entries;
                } else {
                    T_W("Could not delete extra dynamic entries");
                    return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
                }
            /* Need to check if there are any entries in the snooping table that need to be added
               since the limit is being set higher. */
            } else if (curr_port_conf.curr_dynamic_entries < port_conf->max_dynamic_entries) {
                update_dynamic_entries = true;
            }
        }
    } else {
        new_port_conf.curr_dynamic_entries = 0;
        new_port_conf.link_local_qce_id = VTSS_APPL_QOS_QCE_ID_NONE;
    }

    new_port_conf.conf = *port_conf;

    if (port_found && (port_conf->enabled != curr_port_conf.conf.enabled)) {
        mode_changed = true;
    }

    if (ipv6_source_guard_port_configuration.set(ifindex, &new_port_conf) != MESA_RC_OK) {
        T_W("Port configuration could not be set. Exit");
        return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
    }

    if (mode_changed && module_globally_enabled) {
        if (port_conf->enabled) {
            if (ipv6_source_guard_port_enable(ifindex) != VTSS_RC_OK) {
                T_W("Could not enable port, exit");
                return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
            }
        } else {
            if (ipv6_source_guard_port_disable(ifindex) != VTSS_RC_OK) {
                T_W("Could not disable port, exit");
                return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
            }
        }
    }

    module_port_enabled = module_globally_enabled && port_conf->enabled;

    /* Update dynamic entries if port limit was being set higher. */
    if (!mode_changed && module_port_enabled && update_dynamic_entries) {
        port_no = get_port_from_ifindex(ifindex);
        if (ipv6_source_guard_get_snooping_entries(port_no, false, NULL) != VTSS_RC_OK) {
            T_D("Could not add dynamic entries for port, exit");
            return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
        }
    }

    T_D("exit");
    return VTSS_RC_OK;
}

/* 
 * Get static table entry 
 */
static mesa_rc ipv6_source_guard_static_entry_data_get(
    const vtss_appl_ipv6_source_guard_entry_index_t  *const static_index,
    vtss_appl_ipv6_source_guard_entry_data_t         *const static_data)
{
    vtss_appl_ipv6_source_guard_entry_qce_data_t data;

    T_D("enter");

    mesa_rc rc = ipv6_source_guard_static_entries.get(*static_index, &data);
    memcpy(static_data->mac_addr.addr, data.mac_addr.addr, sizeof(data.mac_addr.addr));

    T_D("exit");
    return rc;
}

/* 
 * Set static table entry 
 */
static mesa_rc ipv6_source_guard_static_entry_set(
    const vtss_appl_ipv6_source_guard_entry_index_t  *const static_index,
    const vtss_appl_ipv6_source_guard_entry_data_t   *const static_data)
{
    mesa_port_no_t port_no;
    vtss_appl_ipv6_source_guard_entry_qce_data_t temp_entry_data;
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter");

    /* Check if all parameters are legal */
    if (static_index == nullptr) {
        T_D("Invalid index parameter, exit");
        return IPV6_SOURCE_GUARD_ERROR_INV_PARAMETER;
    }

    if (!vtss_ifindex_is_port(static_index->ifindex)) {
        T_D("Invalid interface index, exit");
        return IPV6_SOURCE_GUARD_ERROR_INV_PARAMETER;
    }

    if ((rc = ipv6_source_guard_check_valid_static_address(static_index->ipv6_addr)) != VTSS_RC_OK) {
        T_D("%s", ipv6_source_guard_error_txt(rc));
        return rc;
    }

    port_no = get_port_from_ifindex(static_index->ifindex);

    /* Check if entry already exists in static table. */
    if ((rc = ipv6_source_guard_static_entries.get(*static_index, &temp_entry_data)) == MESA_RC_OK) {
        T_D("Static entry already exists, exit");
        return IPV6_SOURCE_GUARD_ERROR_ENTRY_ALREADY_IN_TABLE;
    }
    
    /* Check if entry already exists in dynamic table. */
    if ((rc = ipv6_source_guard_dynamic_entries.get(*static_index, &temp_entry_data)) == MESA_RC_OK) {
        if (memcmp(temp_entry_data.mac_addr.addr, static_data->mac_addr.addr, 
            sizeof(temp_entry_data.mac_addr.addr)) != 0) {
            T_W("Mac address does not fit dynamic entry mac, exit");
            return IPV6_SOURCE_GUARD_ERROR_INV_PARAMETER;
        }
        if ((rc = ipv6_source_guard_translate_dynamic_to_static(*static_index)) == MESA_RC_OK) {
            T_D("Dynamic entry successfully translated, exit");
        } else {
            T_D("Dynamic entry could not be translated, exit");
        }
        return rc;
    }

    /* Check if total number of binding entries has reached max allowed. */
    if (get_total_number_of_entries() >= IPV6_SOURCE_GUARD_MAX_ENTRY_CNT) {
        T_W("Ipv6 source guard: entry table full, exit");
        return IPV6_SOURCE_GUARD_ERROR_BINDING_TABLE_FULL;
    }

    temp_entry_data.qce_id = VTSS_APPL_QOS_QCE_ID_NONE;
    temp_entry_data.mac_addr = static_data->mac_addr;

    /* Adding entry to static table */
    if ((rc = ipv6_source_guard_static_entries.set(*static_index, &temp_entry_data)) != MESA_RC_OK) {
        T_W("Could not add entry to static table, exit");
        return rc;
    }

    /* Check if module and port are enabled. */
    if ((rc = ipv6_source_guard_global_port_enabled_check(static_index->ifindex)) != MESA_RC_OK) {
        T_D("Port/Module not enabled, exit");
        return VTSS_RC_OK;
    }

    /* Creating new qce entry. */
    if ((rc = ipv6_source_guard_qce_binding_entry_add(*static_index, &temp_entry_data, port_no)) != MESA_RC_OK) {
        T_W("Could not create qce rule for entry, exit");
        return rc;
    }

    /* Updating entry in static table with the qce_id */
    if ((rc = ipv6_source_guard_static_entries.set(*static_index, &temp_entry_data)) != MESA_RC_OK) {
        T_D("Could not update entry in static table, exit");
        return rc;
    }

    T_D("exit");
    return VTSS_RC_OK;
}

/* 
 * Delete static table entry 
 */
static mesa_rc ipv6_source_guard_static_entry_del(
    const vtss_appl_ipv6_source_guard_entry_index_t  *const static_index)
{
    vtss_appl_ipv6_source_guard_entry_qce_data_t data;
    mesa_rc rc;
    
    T_D("enter");
    
    if (ipv6_source_guard_static_entries.get(*static_index, &data) != MESA_RC_OK) {
        T_D("Entry did not exist in static table, exit");
        return VTSS_RC_OK;
    }

    /* Check if there is an QCE rule for this entry. */
    if (data.qce_id != VTSS_APPL_QOS_QCE_ID_NONE) {
        if (ipv6_source_guard_qce_binding_entry_delete(data.qce_id) != MESA_RC_OK) {
            T_W("ACE rule could not be deleted, exit");
            return IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL;
        }
    }
    rc = ipv6_source_guard_static_entries.del(*static_index);

    /* If source guard is enabled globally and on port we need to check if the entry that was deleted exists in dhcp6 snooping table,
       and if so add it as a dynamic entry if dynamic port limit has not been reached. */
    if (rc == MESA_RC_OK) {
        rc = ipv6_source_guard_global_port_enabled_check(static_index->ifindex);
        if (rc != MESA_RC_OK) {
            T_D("Module/port not enabled, exit");
            return VTSS_RC_OK;
        }

        ipv6_source_guard_check_snooping_entries(static_index);
    }

    T_D("exit");
    return rc;
}

/* 
 * Get dynamic table entry 
 */
static mesa_rc ipv6_source_guard_dynamic_entry_data_get(
    const vtss_appl_ipv6_source_guard_entry_index_t *const dynamic_index,
    vtss_appl_ipv6_source_guard_entry_data_t        *const dynamic_data)
{
    vtss_appl_ipv6_source_guard_entry_qce_data_t data;

    T_D("enter");

    mesa_rc rc = ipv6_source_guard_dynamic_entries.get(*dynamic_index, &data);
    memcpy(dynamic_data->mac_addr.addr, data.mac_addr.addr, sizeof(data.mac_addr.addr));
    
    T_D("exit");
    return rc;
}

/*************************************************************************
** Official API Functions
*************************************************************************/

/**
 * \brief Iterate through all ports where IPv6 Source Guard is enabled.
 * \param prev      [IN] Provide null-pointer to get the first ifindex,
 *                       otherwise a pointer to the current ifindex.
 * \param next      [OUT] First/next ifindex.
 * \return VTSS_RC_OK if the operation succeeded, 
 *         VTSS_RC_ERROR if no "next" ifinedx exists and the end has been reached.
 */
mesa_rc vtss_appl_ipv6_source_guard_port_conf_itr(
    const vtss_ifindex_t    *const prev,
    vtss_ifindex_t          *const next) 
{
    return ipv6_source_guard_port_conf_itr(prev, next);
}

/**
 * \brief Iterate through all the static binding entries.
 * \param prev      [IN] Provide null-pointer to get the first entry,
 *                       otherwise a pointer to the current entry.
 * \param next      [OUT] First/next table index.
 * \return VTSS_RC_OK if the operation succeeded
 *         VTSS_RC_ERROR if no "next" entry exists and the end has been reached.
 */
mesa_rc  vtss_appl_ipv6_source_guard_static_entry_itr(
    const vtss_appl_ipv6_source_guard_entry_index_t  *const prev,
    vtss_appl_ipv6_source_guard_entry_index_t        *const next) 
{
    return ipv6_source_guard_static_entry_itr(prev, next);
}

/**
 * \brief Iterate through all the dynamic binding entries.
 * \param prev      [IN] Provide null-pointer to get the first entry,
 *                       otherwise a pointer to the current entry.
 * \param next      [OUT] First/next table index.
 * \return VTSS_RC_OK if the operation succeeded
 *         VTSS_RC_ERROR if no "next" entry exists and the end has been reached.
 */
mesa_rc vtss_appl_ipv6_source_guard_dynamic_entry_itr(
    const vtss_appl_ipv6_source_guard_entry_index_t  *const prev,
    vtss_appl_ipv6_source_guard_entry_index_t        *const next)
{
    return ipv6_source_guard_dynamic_entry_itr(prev, next);
}

/**
 * \brief Get global configuration of IPv6 Source Guard.
 * \param conf      [OUT] Global configuration of IPv6 Source Guard.
 * \return VTSS_RC_OK if the operation succeeded, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_global_config_get(
    vtss_appl_ipv6_source_guard_global_config_t    *const conf)
{
    return ipv6_source_guard_global_config_get(conf);
}

/**
 * \brief Set or modify global configuration of IPv6 Source Guard.
 * \param conf      [IN] Global configuration of IPv6 Source Guard.
 * \return VTSS_RC_OK if the operation succeeded, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_global_config_set(
    const vtss_appl_ipv6_source_guard_global_config_t  *const conf)
{
    return ipv6_source_guard_global_config_set(conf);
}

/**
 * \brief Get port configuration of IPv6 Source Guard.
 * \param ifindex   [IN] Interface index - the logical interface 
 *                       index of the physical port.
 * \param port_conf [OUT] The current configuration of the port.
 * \return VTSS_RC_OK if the operation succeeded, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_port_config_get(
    vtss_ifindex_t  ifindex,
    vtss_appl_ipv6_source_guard_port_config_t  *const port_conf)
{
    return ipv6_source_guard_port_config_get(ifindex, port_conf);
}

/**
 * \brief Set or modify port configuration of IPv6 Source Guard.
 * \param ifindex   [IN] Interface index - the logical interface index
 *                       of the physical port.
 * \param port_conf [IN] The configuration set to the port.
 * \return VTSS_RC_OK if the operation succeeded, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_port_config_set(
    vtss_ifindex_t  ifindex,
    const vtss_appl_ipv6_source_guard_port_config_t   *const port_conf)
{
    return ipv6_source_guard_port_config_set(ifindex, port_conf);
}

/**
 * \brief Get static binding entry data of IPv6 Source Guard.
 * \param static_index   [IN] Table index of static binding entry.
 * \param static_data    [OUT] Data of static binding entry.
 * \return VTSS_RC_OK if the operation succeeded, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_static_entry_data_get(
    const vtss_appl_ipv6_source_guard_entry_index_t  *const static_index,
    vtss_appl_ipv6_source_guard_entry_data_t         *const static_data)
{
    return ipv6_source_guard_static_entry_data_get(static_index, static_data);
}

/**
 * \brief Add or modify static binding entry of IPv6 Source Guard.
 * \param static_index   [IN] Table index of static binding entry.
 * \param static_data    [IN] Data of static binding entry.
 * \return VTSS_RC_OK if the operation succeeded, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_static_entry_set(
    const vtss_appl_ipv6_source_guard_entry_index_t  *const static_index,
    const vtss_appl_ipv6_source_guard_entry_data_t   *const static_data)
{
    return ipv6_source_guard_static_entry_set(static_index, static_data);
}

/**
 * \brief Delete static binding entry of IPv6 Source Guard.
 * \param static_index   [IN] Table index of static binding entry.
 * \return VTSS_RC_OK if the operation succeeded, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_static_entry_del(
    const vtss_appl_ipv6_source_guard_entry_index_t  *const static_index)
{
    return ipv6_source_guard_static_entry_del(static_index);
}

/**
 * \brief Get default configuration for static binding entry of IPv6 Source Guard.
 * \param static_index  [OUT] The default table index of static binding entry.
 * \param static_data   [OUT] The default configuration of static binding entry.
 * \return VTSS_RC_OK for success operation, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_static_entry_default(
    vtss_appl_ipv6_source_guard_entry_index_t   *const static_index,
    vtss_appl_ipv6_source_guard_entry_data_t    *const static_data)
{
     if (!static_index || !static_data) {
        T_D("Invalid input, exit");
        return IPV6_SOURCE_GUARD_ERROR_INV_PARAMETER;
    }

    memset(static_index, 0, sizeof(vtss_appl_ipv6_source_guard_entry_index_t));
    memset(static_data, 0, sizeof(vtss_appl_ipv6_source_guard_entry_data_t));

    static_index->vlan_id = 0;

    return VTSS_RC_OK;
}

/**
 * \brief Get dynamic binding entry data of IPv6 Source Guard.
 * \param dynamic_index     [IN] Table index of dynamic binding entry.
 * \param dynamic_data      [OUT] Data of dynamic binding entry.
 * \return VTSS_RC_OK if the operation succeeded, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_dynamic_entry_data_get(
    const vtss_appl_ipv6_source_guard_entry_index_t *const dynamic_index,
    vtss_appl_ipv6_source_guard_entry_data_t        *const dynamic_data)
{
    return ipv6_source_guard_dynamic_entry_data_get(dynamic_index, dynamic_data);
}

/**
 * \brief Get control action of IPv6 Source Guard for translating dynamic entries to static.
 * This action is active only when SET is involved.
 * It always returns FALSE when getting this action data.
 * \param action    [OUT] The IPv6 source guard action data.
 * \return VTSS_RC_OK if the operation succeeded, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_control_translate_get(
    vtss_appl_ipv6_source_guard_control_translate_t   *const action)
{
    action->translate = false;
    return VTSS_RC_OK;
}

/**
 * \brief Set control action of IPv6 Source Guard for translating dynamic entries to static.
 * This action is active only when SET is involved and its value is set to be TRUE.
 * When it is active, it means translating process is taking action.
 * \param action    [IN] The IPv6 source guard action data.
 * \return VTSS_RC_OK if the operation succeeded, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_control_translate_set(
    const vtss_appl_ipv6_source_guard_control_translate_t *const action)
{
    if (action->translate) {
        return ipv6_source_guard_translate_all_dynamic_entries();
    }
    return VTSS_RC_OK;
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

#ifdef VTSS_SW_OPTION_ICLI
extern "C" int ipv6_source_guard_icli_cmd_register();
#endif
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Initialize private mib */
VTSS_PRE_DECLS void ipv6_source_guard_mib_init(void);
#endif

/* Initialize module */
mesa_rc ipv6_source_guard_init(vtss_init_data_t *data) {

    switch (data->cmd) {
        case INIT_CMD_INIT:
            T_D("INIT_CMD_INIT");

            critd_init(&ipv6_source_guard_crit, "ipv6_source_guard", VTSS_MODULE_ID_IPV6_SOURCE_GUARD, CRITD_TYPE_MUTEX);
            IPV6_SOURCE_GUARD_CRIT_ENTER();

/* Activate the management interfaces */
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
            /* Register private mib */
            ipv6_source_guard_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
            vtss_appl_ipv6_source_guard_json_init();
#endif
#ifdef VTSS_SW_OPTION_ICLI
            ipv6_source_guard_icli_cmd_register();
#endif
            /* TODO: Subscribe to dhcpv6 snooping module for dynamic table.*/
            IPV6_SOURCE_GUARD_CRIT_EXIT();

            /* Setting default configuration. */
            if (ipv6_source_guard_set_default_conf() != VTSS_RC_OK) {
                T_W("Module could not be configured. Exit.");
                return IPV6_SOURCE_GUARD_ERROR_DEFAULT_CONFIGURE;
            }

            T_D("INIT_CMD_INIT DONE");
            break;

        case INIT_CMD_START:
            T_D("INIT_CMD_START");

#ifdef VTSS_SW_OPTION_ICFG
        if (ipv6_source_guard_icfg_init() != VTSS_RC_OK) {
            T_D("Calling icfg_init() failed");
        }
#endif
            T_D("INIT_CMD_START DONE");
            break;

        /* Restore default configuration */
        case INIT_CMD_CONF_DEF:
            T_D("INIT_CMD_CONF_DEF");

            if (ipv6_source_guard_reload_default_conf() != VTSS_RC_OK) {
                T_W("Module could not be reloaded. Exit.");
                return IPV6_SOURCE_GUARD_ERROR_DEFAULT_CONFIGURE;
            }

            T_D("INIT_CMD_CONF_DEF");
            break;

        case INIT_CMD_SUSPEND_RESUME:
            IPV6_SOURCE_GUARD_CRIT_ENTER();
            IPV6_SOURCE_GUARD_CRIT_EXIT();
            break;

        default:
            break;

    }
    return VTSS_RC_OK;
}

