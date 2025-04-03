/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "misc_api.h"

#include "ipmc_lib.hxx"
#include "ipmc_lib_trace.h"
#include "ipmc_lib_base.hxx"
#include "ipmc_lib_profile.hxx"
#include <vtss/appl/ipmc_lib.h>
#include "mgmt_api.h"   /* For IPV6_ADDR_IBUF_MAX_LEN */
#include "ip_utils.hxx" /* For ipv6_calc_t            */
#include <vtss/basics/list.hxx>
#include <vtss/basics/map.hxx>
#include "ipmc_lib_profile_observer.hxx"
#if defined(VTSS_SW_OPTION_SYSLOG)
#include "syslog_api.h"
#endif /* VTSS_SW_OPTION_SYSLOG */

// Ranges are standalone and put into its own map sorted alphabetically.
typedef vtss::Map<vtss_appl_ipmc_lib_profile_range_key_t, vtss_appl_ipmc_lib_profile_range_conf_t> ipmc_lib_profile_range_map_t;
typedef ipmc_lib_profile_range_map_t::iterator ipmc_lib_profile_range_itr_t;
static ipmc_lib_profile_range_map_t IPMC_LIB_PROFILE_range_map;

// Rules are ordered and per profile. They reference a range in the range map.
// When retrieving or isnerting rules for a given profile, it is the
// <profile_key, range_key> that is used.
typedef struct {
    // The range that this rule references.
    vtss_appl_ipmc_lib_profile_range_key_t range;

    // The rule itself.
    vtss_appl_ipmc_lib_profile_rule_conf_t conf;
} ipmc_lib_profile_rule_t;

typedef vtss::List<ipmc_lib_profile_rule_t>    ipmc_lib_profile_rule_list_t;
typedef ipmc_lib_profile_rule_list_t::iterator ipmc_lib_profile_rule_itr_t;

// Profiles are put into its own map sorted alphabetically
typedef struct {
    vtss_appl_ipmc_lib_profile_conf_t conf;

    // Ordered list of rules and associated ranges.
    ipmc_lib_profile_rule_list_t rule_list;
} ipmc_lib_profile_state_t;

typedef vtss::Map<vtss_appl_ipmc_lib_profile_key_t, ipmc_lib_profile_state_t> ipmc_lib_profile_map_t;
typedef ipmc_lib_profile_map_t::iterator                                      ipmc_lib_profile_itr_t;

// Whenever a change to a profile or one of its rules changes, a notification is
// sent to the observes of the following object.
ipmc_lib_profile_change_notification_t ipmc_lib_profile_change_notification("ipmc_lib_profile_change_notification", VTSS_MODULE_ID_IPMC_LIB);

static vtss_appl_ipmc_lib_profile_key_t          IPMC_LIB_PROFILE_key_global;
static ipmc_lib_profile_map_t                    IPMC_LIB_PROFILE_map;
static critd_t                                   IPMC_LIB_PROFILE_crit;
static vtss_appl_ipmc_lib_profile_capabilities_t IPMC_LIB_PROFILE_cap;
static vtss_appl_ipmc_lib_profile_global_conf_t  IPMC_LIB_PROFILE_global_default_conf;
static vtss_appl_ipmc_lib_profile_global_conf_t  IPMC_LIB_PROFILE_global_conf;

struct IPMC_LIB_PROFILE_lock {
    IPMC_LIB_PROFILE_lock(const char *file, int line)
    {
        critd_enter(&IPMC_LIB_PROFILE_crit, file, line);
    }

    ~IPMC_LIB_PROFILE_lock()
    {
        critd_exit(&IPMC_LIB_PROFILE_crit, __FILE__, 0);
    }
};

#define IPMC_LIB_PROFILE_LOCK_SCOPE() IPMC_LIB_PROFILE_lock __ipmc_lib_profile_lock_guard__(__FILE__, __LINE__)
#define IPMC_LIB_PROFILE_LOCK_ASSERT_LOCKED(_fmt_, ...) if (!critd_is_locked(&IPMC_LIB_PROFILE_crit)) {T_E(_fmt_, ##__VA_ARGS__);}

// This structure is used when logging something to the syslog.
typedef struct {
    bool is_warning;
    char msg[300];
} ipmc_lib_profile_log_t;

/******************************************************************************/
// IPMC_LIB_PROFILE_log()
/******************************************************************************/
static void IPMC_LIB_PROFILE_log(const ipmc_lib_profile_log_t &log)
{
    char log_buf[sizeof(log.msg) + 100];

    snprintf(log_buf, sizeof(log_buf), "IPMC: Date&Time: %s, %s", misc_time2str(time(NULL)), log.msg);

    T_IG(IPMC_LIB_TRACE_GRP_LOG, "Severity: %s: %s", log.is_warning ? "Warning" : "Normal", log_buf);

#if defined(VTSS_SW_OPTION_SYSLOG)
    if (log.is_warning) {
        S_W("%s", log_buf);
    } else {
        S_I("%s", log_buf);
    }
#endif /* VTSS_SW_OPTION_SYSLOG */
}

/******************************************************************************/
// vtss_appl_ipmc_lib_profile_key_t::operator<
// Used for sorting the profiles in the profile map
/******************************************************************************/
static bool operator<(const vtss_appl_ipmc_lib_profile_key_t &lhs, const vtss_appl_ipmc_lib_profile_key_t &rhs)
{
    // Sort alphabetically
    return strcmp(lhs.name, rhs.name) < 0;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_profile_range_key_t::operator<
// Used for sorting the ranges in the profile map
/******************************************************************************/
static bool operator<(const vtss_appl_ipmc_lib_profile_range_key_t &lhs, const vtss_appl_ipmc_lib_profile_range_key_t &rhs)
{
    // Sort alphabetically
    return strcmp(lhs.name, rhs.name) < 0;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_profile_key_t::operator==()
// Makes it possible to do a "if (profile_key1 == profile_key2)"
/******************************************************************************/
bool operator==(const vtss_appl_ipmc_lib_profile_key_t &lhs, const vtss_appl_ipmc_lib_profile_key_t &rhs)
{
    // Don't use memcmp(), because there may be uncleared bytes beyond the
    // string length in both lhs and rhs.
    return strcmp(lhs.name, rhs.name) == 0;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_profile_key_t::operator!=()
// Makes it possible to do a "if (profile_key1 != profile_key2)"
/******************************************************************************/
static bool operator!=(const vtss_appl_ipmc_lib_profile_key_t &lhs, const vtss_appl_ipmc_lib_profile_key_t &rhs)
{
    return !(lhs == rhs);
}

/******************************************************************************/
// vtss_appl_ipmc_lib_profile_range_key_t::operator==()
// Makes it possible to do a "if (range_key1 == range_key2)"
/******************************************************************************/
bool operator==(const vtss_appl_ipmc_lib_profile_range_key_t &lhs, const vtss_appl_ipmc_lib_profile_range_key_t &rhs)
{
    // Don't use memcmp(), because there may be uncleared bytes beyond the
    // string length in both lhs and rhs.
    return strcmp(lhs.name, rhs.name) == 0;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_profile_rule_conf_t::operator==()
// Makes it possible to do a "if (rule_conf1 == rule_conf2)"
/******************************************************************************/
bool operator==(const vtss_appl_ipmc_lib_profile_rule_conf_t &lhs, const vtss_appl_ipmc_lib_profile_rule_conf_t &rhs)
{
    // Don't use memcmp(), because there may be uncleared bytes beyond the
    // string length in both lhs and rhs.
    return lhs.deny == rhs.deny && lhs.log == rhs.log;
}

/******************************************************************************/
// IPMC_LIB_PROFILE_notif_add()
/******************************************************************************/
static void IPMC_LIB_PROFILE_notif_add(const vtss_appl_ipmc_lib_profile_key_t *key)
{
    mesa_rc rc;

    if ((rc = ipmc_lib_profile_change_notification.set(key, 0)) != VTSS_RC_OK) {
        T_EG(IPMC_LIB_TRACE_GRP_PROFILE, "%s: Unable to create notification: %s", key->name, error_txt(rc));
    }
}

/******************************************************************************/
// IPMC_LIB_PROFILE_notif_set()
/******************************************************************************/
static void IPMC_LIB_PROFILE_notif_set(const vtss_appl_ipmc_lib_profile_key_t *key)
{
    uint32_t val;
    mesa_rc  rc;

    if ((rc = ipmc_lib_profile_change_notification.get(key, &val)) != VTSS_RC_OK) {
        T_EG(IPMC_LIB_TRACE_GRP_PROFILE, "%s: Unable to get notification: %s", key->name, error_txt(rc));
        val = 0;
    }

    // Simply setting anoter value causes an invocation of the observers.
    val++;

    if ((rc = ipmc_lib_profile_change_notification.set(key, val)) != VTSS_RC_OK) {
        T_EG(IPMC_LIB_TRACE_GRP_PROFILE, "%s: Unable to set notification to %u: %s", key->name, val, error_txt(rc));
    }
}

/******************************************************************************/
// IPMC_LIB_PROFILE_notif_del()
/******************************************************************************/
static void IPMC_LIB_PROFILE_notif_del(const vtss_appl_ipmc_lib_profile_key_t *key)
{
    mesa_rc rc;

    if ((rc = ipmc_lib_profile_change_notification.del(key)) != VTSS_RC_OK) {
        T_EG(IPMC_LIB_TRACE_GRP_PROFILE, "%s: Unable to delete notification: %s", key->name, error_txt(rc));
    }
}

/******************************************************************************/
// IPMC_LIB_PROFILE_ptr_check()
/******************************************************************************/
static mesa_rc IPMC_LIB_PROFILE_ptr_check(const void *ptr)
{
    return ptr == nullptr ? (mesa_rc)VTSS_APPL_IPMC_LIB_RC_INVALID_PARAMETER : VTSS_RC_OK;
}

/******************************************************************************/
// IPMC_LIB_PROFILE_name_check()
/******************************************************************************/
static mesa_rc IPMC_LIB_PROFILE_name_check(const char *name, bool is_profile, bool allow_empty)
{
    size_t len, i;

    len = strlen(name);

    if (len == 0) {
        // An empty name is not OK, unless allow_empty is true
        return allow_empty ? VTSS_RC_OK : is_profile ? (mesa_rc)VTSS_APPL_IPMC_LIB_RC_PROFILE_NAME_EMPTY : (mesa_rc)VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_NAME_EMPTY;
    }

    if (len > VTSS_APPL_IPMC_LIB_PROFILE_NAME_LEN_MAX) {
        return is_profile ? VTSS_APPL_IPMC_LIB_RC_PROFILE_NAME_TOO_LONG : VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_NAME_TOO_LONG;
    }

    for (i = 0; i < len; i++) {
        if (!isgraph(name[i])) {
            T_DG(IPMC_LIB_TRACE_GRP_PROFILE, "Invalid char: %c", name[i]);
            return is_profile ? VTSS_APPL_IPMC_LIB_RC_PROFILE_NAME_CONTENTS : VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_NAME_CONTENTS;
        }
    }

    // "all" is reserved in ICLI. Using case-insensitive match, because ICLI is
    // case-insensitive.
    if (strcasecmp(name, "all") == 0) {
        return is_profile ? VTSS_APPL_IPMC_LIB_RC_PROFILE_NAME_CONTENTS_ALL : VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_NAME_CONTENTS_ALL;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// IPMC_LIB_PROFILE_dscr_check()
/******************************************************************************/
static mesa_rc IPMC_LIB_PROFILE_dscr_check(const char *dscr)
{
    size_t len, i;

    len = strlen(dscr);

    if (len == 0) {
        // An empty description is OK.
        return VTSS_RC_OK;
    }

    if (len > VTSS_APPL_IPMC_LIB_PROFILE_DSCR_LEN_MAX) {
        return VTSS_APPL_IPMC_LIB_RC_PROFILE_DSCR_TOO_LONG;
    }

    for (i = 0; i < len; i++) {
        if (!isgraph(dscr[i]) && dscr[i] != ' ') {
            T_DG(IPMC_LIB_TRACE_GRP_PROFILE, "Invalid char: %c", dscr[i]);
            return VTSS_APPL_IPMC_LIB_RC_PROFILE_DSCR_CONTENTS;
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// IPMC_LIB_PROFILE_type_check()
/******************************************************************************/
static mesa_rc IPMC_LIB_PROFILE_type_check(vtss_appl_ipmc_lib_profile_range_type_t type)
{
    if (type == VTSS_APPL_IPMC_LIB_PROFILE_RANGE_TYPE_IPV4 ||
        type == VTSS_APPL_IPMC_LIB_PROFILE_RANGE_TYPE_ANY) {
        return VTSS_RC_OK;
    }

    if (type == VTSS_APPL_IPMC_LIB_PROFILE_RANGE_TYPE_IPV6) {
#if defined(VTSS_SW_OPTION_IPV6)
        return VTSS_RC_OK;
#else
        return VTSS_APPL_IPMC_LIB_RC_MLD_NOT_SUPPORTED;
#endif
    }

    return VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_INVALID_TYPE;
}

/******************************************************************************/
// IPMC_LIB_PROFILE_del()
/******************************************************************************/
static void IPMC_LIB_PROFILE_del(ipmc_lib_profile_itr_t profile_itr)
{
    vtss_appl_ipmc_lib_profile_key_t key = profile_itr->first;

    // Delete all rules from the profile
    profile_itr->second.rule_list.clear();

    // Delete the entry from our map.
    IPMC_LIB_PROFILE_map.erase(key);

    // Create a change notification
    IPMC_LIB_PROFILE_notif_del(&key);
}

/******************************************************************************/
// IPMC_LIB_PROFILE_conf_default()
/******************************************************************************/
static void IPMC_LIB_PROFILE_conf_default(void)
{
    ipmc_lib_profile_itr_t profile_itr, next_profile_itr;

    IPMC_LIB_PROFILE_LOCK_SCOPE();

    // To limit the number of callbacks to observers of profile changes, we
    // first clear the profile map and then the range map.
    profile_itr = IPMC_LIB_PROFILE_map.begin();
    while (profile_itr != IPMC_LIB_PROFILE_map.end()) {
        next_profile_itr = profile_itr;
        ++next_profile_itr;

        IPMC_LIB_PROFILE_del(profile_itr);
        profile_itr = next_profile_itr;
    }

    IPMC_LIB_PROFILE_range_map.clear();

    IPMC_LIB_PROFILE_global_conf = IPMC_LIB_PROFILE_global_default_conf;
}

/******************************************************************************/
// IPMC_LIB_PROFILE_capabilities_set()
// This is the only place where we define maximum values for various parameters,
// so if you need different max. values, change here - only!
/******************************************************************************/
static void IPMC_LIB_PROFILE_capabilities_set(void)
{
    IPMC_LIB_PROFILE_cap.profile_cnt_max =  64;
    IPMC_LIB_PROFILE_cap.range_cnt_max   = 128;
}

/******************************************************************************/
// IPMC_LIB_PROFILE_global_default_conf_set()
/******************************************************************************/
static void IPMC_LIB_PROFILE_global_default_conf_set(void)
{
    vtss_clear(IPMC_LIB_PROFILE_global_default_conf);
}

/******************************************************************************/
// ipmc_lib_profile_permit()
// Returns true if the IP address is allowed when using this particular profile.
/******************************************************************************/
bool ipmc_lib_profile_permit(const ipmc_lib_vlan_state_t &vlan_state, const ipmc_lib_profile_match_t &match, bool log)
{
    char                         dip_buf[IPV6_ADDR_IBUF_MAX_LEN], sip_buf[IPV6_ADDR_IBUF_MAX_LEN];
    char                         if_str[40];
    ipmc_lib_profile_itr_t       profile_itr;
    ipmc_lib_profile_rule_itr_t  rule_itr;
    ipmc_lib_profile_range_itr_t range_itr;
    ipmc_lib_profile_log_t       profile_log;
    ipv6_calc_t                  dip, sip, start, end;
    bool                         permit, do_log;

    // Convert to an object that we can do computations on. Despite its name,
    // the object works for both IPv4 and IPv6 addresses.
    dip = match.dst.is_ipv4 ? ipv6_calc_t(match.dst.ipv4) : ipv6_calc_t(match.dst.ipv6);

    IPMC_LIB_PROFILE_LOCK_SCOPE();

    if (!IPMC_LIB_PROFILE_global_conf.enable) {
        return vlan_state.vlan_key.is_mvr ? false /* Deny, because MVR must have IPMC profiles globally enabled */ : true /* Permit, because IPMC doesn't need IPMC profiles */;
    }

    if (match.profile_key.name[0] == '\0') {
        T_IG(IPMC_LIB_TRACE_GRP_PROFILE, "%s: Profile name is empty when looking up %s", vlan_state.vlan_key, dip.print(dip_buf, match.dst.is_ipv4));
        return vlan_state.vlan_key.is_mvr ? false /* Deny, because MVR must have IPMC profiles attached */ : true /* Permit, because IPMC doesn't need IPMC profiles */;
    }

    if ((profile_itr = IPMC_LIB_PROFILE_map.find(match.profile_key)) == IPMC_LIB_PROFILE_map.end()) {
        T_IG(IPMC_LIB_TRACE_GRP_PROFILE, "Profile %s not defined when looking up %s", match.profile_key.name, dip.print(dip_buf, match.dst.is_ipv4));
        return false; // Deny
    }

    permit = false;
    do_log = false;
    for (rule_itr = profile_itr->second.rule_list.begin(); rule_itr != profile_itr->second.rule_list.end(); ++rule_itr) {
        if ((range_itr = IPMC_LIB_PROFILE_range_map.find(rule_itr->range)) == IPMC_LIB_PROFILE_range_map.end()) {
            T_EG(IPMC_LIB_TRACE_GRP_PROFILE, "Internal error: Profile %s has a rule which references range %s, which doesn't exist - when looking up %s", match.profile_key.name, rule_itr->range.name, dip.print(dip_buf, match.dst.is_ipv4));
            return false; // Deny
        }

        if (match.dst.is_ipv4 != range_itr->second.start.is_ipv4) {
            // One is IPv4, the other is IPv6
            continue;
        }

        start = match.dst.is_ipv4 ? ipv6_calc_t(range_itr->second.start.ipv4) : ipv6_calc_t(range_itr->second.start.ipv6);
        end   = match.dst.is_ipv4 ? ipv6_calc_t(range_itr->second.end.ipv4)   : ipv6_calc_t(range_itr->second.end.ipv6);

        if (dip < start || dip > end) {
            // Doesn't fit in range
            continue;
        }

        // Hit!
        do_log = rule_itr->conf.log;
        permit = !rule_itr->conf.deny;
        break;
    }

    if (rule_itr == profile_itr->second.rule_list.end()) {
        // No matching rule.
        return false; // Deny
    }

    if (log && do_log) {
        sip = match.src.is_ipv4 ? ipv6_calc_t(match.src.ipv4) : ipv6_calc_t(match.src.ipv6);
        profile_log.is_warning = false;
        snprintf(profile_log.msg, sizeof(profile_log.msg),
                 "Version: %s, DIP: %s, SIP: %s, VLAN ID: %u, Interface: %s, Permit: %s, Profile: %s, Range: %s.",
                 match.dst.is_ipv4 ? "IGMP" : "MLD",
                 dip.print(dip_buf, match.dst.is_ipv4),
                 sip.print(sip_buf, match.src.is_ipv4),
                 match.vid,
                 icli_port_info_txt_short(VTSS_USID_START, iport2uport(match.port_no), if_str),
                 permit ? "Yes" : "No",
                 match.profile_key.name,
                 rule_itr->range.name);
        IPMC_LIB_PROFILE_log(profile_log);
    }

    return permit;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_profile_capabilities_get()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_profile_capabilities_get(vtss_appl_ipmc_lib_profile_capabilities_t *cap)
{
    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(cap));
    *cap = IPMC_LIB_PROFILE_cap;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_profile_global_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_profile_global_conf_default_get(vtss_appl_ipmc_lib_profile_global_conf_t *conf)
{
    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(conf));

    *conf = IPMC_LIB_PROFILE_global_default_conf;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_profile_global_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_profile_global_conf_get(vtss_appl_ipmc_lib_profile_global_conf_t *conf)
{
    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(conf));

    IPMC_LIB_PROFILE_LOCK_SCOPE();
    *conf = IPMC_LIB_PROFILE_global_conf;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_profile_global_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_profile_global_conf_set(const vtss_appl_ipmc_lib_profile_global_conf_t *conf)
{
    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(conf));

    {
        IPMC_LIB_PROFILE_LOCK_SCOPE();

        if (IPMC_LIB_PROFILE_global_conf.enable == conf->enable) {
            return VTSS_RC_OK;
        }

        IPMC_LIB_PROFILE_global_conf = *conf;

        // Notify the special "all" profile. See INIT_CMD_INIT for a description
        // of why.
        IPMC_LIB_PROFILE_notif_set(&IPMC_LIB_PROFILE_key_global);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// ipmc_lib_profile_key_check()
/******************************************************************************/
mesa_rc ipmc_lib_profile_key_check(const vtss_appl_ipmc_lib_profile_key_t *key, bool allow_empty)
{
    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(key));
    return IPMC_LIB_PROFILE_name_check(key->name, true /* this is a profile key */, allow_empty);
}

/******************************************************************************/
// vtss_appl_ipmc_lib_profile_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_profile_conf_get(const vtss_appl_ipmc_lib_profile_key_t *key, vtss_appl_ipmc_lib_profile_conf_t *conf)
{
    ipmc_lib_profile_itr_t itr;

    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(key));
    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(conf));

    T_DG(IPMC_LIB_TRACE_GRP_PROFILE, "key = %s", key->name);

    IPMC_LIB_PROFILE_LOCK_SCOPE();

    if ((itr = IPMC_LIB_PROFILE_map.find(*key)) == IPMC_LIB_PROFILE_map.end()) {
        return VTSS_APPL_IPMC_LIB_RC_PROFILE_NO_SUCH;
    }

    *conf = itr->second.conf;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_profile_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_profile_conf_set(const vtss_appl_ipmc_lib_profile_key_t *key, const vtss_appl_ipmc_lib_profile_conf_t *conf)
{
    ipmc_lib_profile_itr_t itr;
    bool                   new_profile;

    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(key));
    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(conf));
    VTSS_RC(IPMC_LIB_PROFILE_name_check(key->name, true /* this is a profile key */, false /* don't allow it to be empty */));
    VTSS_RC(IPMC_LIB_PROFILE_dscr_check(conf->dscr));

    T_DG(IPMC_LIB_TRACE_GRP_PROFILE, "key = %s, dscr = %s", key->name, conf->dscr);

    IPMC_LIB_PROFILE_LOCK_SCOPE();

    if ((itr = IPMC_LIB_PROFILE_map.find(*key)) == IPMC_LIB_PROFILE_map.end()) {
        // New profile. Check that we have room for it.
        if (IPMC_LIB_PROFILE_map.size() >= IPMC_LIB_PROFILE_cap.profile_cnt_max) {
            return VTSS_APPL_IPMC_LIB_RC_PROFILE_LIMIT_REACHED;
        }

        new_profile = true;
    } else {
        // Don't use memcmp(), because there may be uncleared values beyond the
        // string length in both the existing and the new conf.
        if (strcmp(itr->second.conf.dscr, conf->dscr) == 0) {
            // No changes
            return VTSS_RC_OK;
        }

        new_profile = false;
    }

    // Create a new or get existing
    if ((itr = IPMC_LIB_PROFILE_map.get(*key)) == IPMC_LIB_PROFILE_map.end()) {
        return VTSS_APPL_IPMC_LIB_RC_OUT_OF_MEMORY;
    }

    itr->second.conf = *conf;

    // Create a change notification.
    if (new_profile) {
        IPMC_LIB_PROFILE_notif_add(key);
    } else {
        IPMC_LIB_PROFILE_notif_set(key);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_profile_conf_del()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_profile_conf_del(const vtss_appl_ipmc_lib_profile_key_t *key)
{
    ipmc_lib_profile_itr_t profile_itr;

    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(key));

    T_DG(IPMC_LIB_TRACE_GRP_PROFILE, "key = %s", key->name);

    IPMC_LIB_PROFILE_LOCK_SCOPE();

    if ((profile_itr = IPMC_LIB_PROFILE_map.find(*key)) == IPMC_LIB_PROFILE_map.end()) {
        return VTSS_APPL_IPMC_LIB_RC_PROFILE_NO_SUCH;
    }

    IPMC_LIB_PROFILE_del(profile_itr);

    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_ipmc_lib_profile_itr()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_profile_itr(const vtss_appl_ipmc_lib_profile_key_t *prev, vtss_appl_ipmc_lib_profile_key_t *next)
{
    ipmc_lib_profile_itr_t itr;

    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(next));

    IPMC_LIB_PROFILE_LOCK_SCOPE();

    if (prev) {
        // Here we have a valid prev. Find the next from that one.
        itr = IPMC_LIB_PROFILE_map.greater_than(*prev);
    } else {
        // We don't have a valid prev. Get the first.
        itr = IPMC_LIB_PROFILE_map.begin();
    }

    if (itr != IPMC_LIB_PROFILE_map.end()) {
        *next = itr->first;
        return VTSS_RC_OK;
    }

    // No next
    return VTSS_RC_ERROR;
}

//*****************************************************************************/
// vtss_appl_ipmc_lib_profile_range_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_profile_range_conf_get(const vtss_appl_ipmc_lib_profile_range_key_t *key, vtss_appl_ipmc_lib_profile_range_conf_t *conf)
{
    ipmc_lib_profile_range_itr_t range_itr;

    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(key));
    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(conf));

    T_DG(IPMC_LIB_TRACE_GRP_PROFILE, "key = %s", key->name);

    IPMC_LIB_PROFILE_LOCK_SCOPE();

    if ((range_itr = IPMC_LIB_PROFILE_range_map.find(*key)) == IPMC_LIB_PROFILE_range_map.end()) {
        return VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_NO_SUCH;
    }

    *conf = range_itr->second;
    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_ipmc_lib_profile_range_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_profile_range_conf_set(const vtss_appl_ipmc_lib_profile_range_key_t *key, const vtss_appl_ipmc_lib_profile_range_conf_t *conf)
{
    ipmc_lib_profile_itr_t       profile_itr;
    ipmc_lib_profile_rule_itr_t  rule_itr;
    ipmc_lib_profile_range_itr_t range_itr;
    bool                         new_range;

    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(key));
    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(conf));
    VTSS_RC(IPMC_LIB_PROFILE_name_check(key->name, false /* return range errors */, false /* don't allow it to be empty */));

    T_DG(IPMC_LIB_TRACE_GRP_PROFILE, "key = %s, conf = %s", key->name, *conf);

    if (conf->start.is_ipv4 != conf->end.is_ipv4) {
        return VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_START_AND_END_NOT_SAME_IP_VERSION;
    }

    if (conf->start.is_ipv4) {
        // Check IPv4 addresses are multicast.
        if (!conf->start.is_mc()) {
            return VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_START_NOT_IPV4_MC;
        }

        if (!conf->end.is_mc()) {
            return VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_END_NOT_IPV4_MC;
        }

        if (conf->start.ipv4 > conf->end.ipv4) {
            return VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_START_GREATER_THAN_END_IPV4;
        }
    } else {
        // Check IPv6 addresses are multicast.
        if (!conf->start.is_mc()) {
            return VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_START_NOT_IPV6_MC;
        }

        if (!conf->end.is_mc()) {
            return VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_END_NOT_IPV6_MC;
        }

        if (conf->start.ipv6 > conf->end.ipv6) {
            return VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_START_GREATER_THAN_END_IPV6;
        }
    }

    IPMC_LIB_PROFILE_LOCK_SCOPE();

    if ((range_itr = IPMC_LIB_PROFILE_range_map.find(*key)) == IPMC_LIB_PROFILE_range_map.end()) {
        // New range. Check that we have room for it.
        if (IPMC_LIB_PROFILE_range_map.size() >= IPMC_LIB_PROFILE_cap.range_cnt_max) {
            return VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_LIMIT_REACHED;
        }

        new_range = true;
    } else {
        if (memcmp(&range_itr->second, conf, sizeof(range_itr->second)) == 0) {
            // No changes
            return VTSS_RC_OK;
        }

        new_range = false;
    }

    // Create a new or get existing
    if ((range_itr = IPMC_LIB_PROFILE_range_map.get(*key)) == IPMC_LIB_PROFILE_range_map.end()) {
        return VTSS_APPL_IPMC_LIB_RC_OUT_OF_MEMORY;
    }

    range_itr->second = *conf;

    // Figure out whether this affects any profiles, if this is an existing
    // range
    if (!new_range) {
        for (profile_itr = IPMC_LIB_PROFILE_map.begin(); profile_itr != IPMC_LIB_PROFILE_map.end(); ++profile_itr) {
            for (rule_itr = profile_itr->second.rule_list.begin(); rule_itr != profile_itr->second.rule_list.end(); ++rule_itr) {
                if (rule_itr->range == *key) { // Using operator==()
                    IPMC_LIB_PROFILE_notif_set(&profile_itr->first);
                    break;
                }
            }
        }
    }

    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_ipmc_lib_profile_range_conf_del()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_profile_range_conf_del(const vtss_appl_ipmc_lib_profile_range_key_t *key)
{
    ipmc_lib_profile_itr_t       profile_itr;
    ipmc_lib_profile_rule_itr_t  rule_itr, next_rule_itr;
    ipmc_lib_profile_range_itr_t range_itr;

    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(key));

    T_DG(IPMC_LIB_TRACE_GRP_PROFILE, "key = %s", key->name);

    IPMC_LIB_PROFILE_LOCK_SCOPE();

    if ((range_itr = IPMC_LIB_PROFILE_range_map.find(*key)) == IPMC_LIB_PROFILE_range_map.end()) {
        return VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_NO_SUCH;
    }

    // Delete the entry from our map.
    IPMC_LIB_PROFILE_range_map.erase(*key);

    // Silently delete all rules referencing this range while notifying
    // observers.
    for (profile_itr = IPMC_LIB_PROFILE_map.begin(); profile_itr != IPMC_LIB_PROFILE_map.end(); ++profile_itr) {
        rule_itr = profile_itr->second.rule_list.begin();
        while (rule_itr != profile_itr->second.rule_list.end()) {
            next_rule_itr = rule_itr;
            ++next_rule_itr;

            if (rule_itr->range == *key) { // Using operator==()
                // Delete rule
                profile_itr->second.rule_list.erase(rule_itr);

                // Notify
                IPMC_LIB_PROFILE_notif_set(&profile_itr->first);

                // Nothing else to do
                break;
            }

            rule_itr = next_rule_itr;
        }
    }

    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_ipmc_lib_profile_range_itr()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_profile_range_itr(const vtss_appl_ipmc_lib_profile_range_key_t *prev, vtss_appl_ipmc_lib_profile_range_key_t *next, vtss_appl_ipmc_lib_profile_range_type_t type)
{
    ipmc_lib_profile_range_itr_t itr;

    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(next));
    VTSS_RC(IPMC_LIB_PROFILE_type_check(type));

    IPMC_LIB_PROFILE_LOCK_SCOPE();

    if (prev) {
        itr = IPMC_LIB_PROFILE_range_map.greater_than(*prev);
    } else {
        itr = IPMC_LIB_PROFILE_range_map.begin();
    }

    while (itr != IPMC_LIB_PROFILE_range_map.end()) {
        if (type  == VTSS_APPL_IPMC_LIB_PROFILE_RANGE_TYPE_ANY                                 ||
            (type == VTSS_APPL_IPMC_LIB_PROFILE_RANGE_TYPE_IPV4 &&  itr->second.start.is_ipv4) ||
            (type == VTSS_APPL_IPMC_LIB_PROFILE_RANGE_TYPE_IPV6 && !itr->second.start.is_ipv4)) {
            *next = itr->first;
            return VTSS_RC_OK;
        }

        ++itr;
    }

    return VTSS_RC_ERROR;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_profile_rule_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_profile_rule_conf_default_get(vtss_appl_ipmc_lib_profile_rule_conf_t *rule_conf)
{
    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(rule_conf));

    rule_conf->deny = true;
    rule_conf->log  = false;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_profile_rule_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_profile_rule_conf_get(const vtss_appl_ipmc_lib_profile_key_t *profile_key, const vtss_appl_ipmc_lib_profile_range_key_t *range_key, vtss_appl_ipmc_lib_profile_rule_conf_t *rule_conf)
{
    ipmc_lib_profile_itr_t      profile_itr;
    ipmc_lib_profile_rule_itr_t rule_itr;

    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(profile_key));
    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(range_key));
    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(rule_conf));

    T_DG(IPMC_LIB_TRACE_GRP_PROFILE, "profile_key = %s, range_key = %s", profile_key->name, range_key->name);

    IPMC_LIB_PROFILE_LOCK_SCOPE();

    if ((profile_itr = IPMC_LIB_PROFILE_map.find(*profile_key)) == IPMC_LIB_PROFILE_map.end()) {
        return VTSS_APPL_IPMC_LIB_RC_PROFILE_NO_SUCH;
    }

    for (rule_itr = profile_itr->second.rule_list.begin(); rule_itr != profile_itr->second.rule_list.end(); ++rule_itr) {
        if (rule_itr->range == *range_key) { // Using operator==()
            break;
        }
    }

    if (rule_itr == profile_itr->second.rule_list.end()) {
        return VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_RULE_NO_SUCH;
    }

    *rule_conf = rule_itr->conf;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_profile_rule_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_profile_rule_conf_set(const vtss_appl_ipmc_lib_profile_key_t *profile_key, const vtss_appl_ipmc_lib_profile_range_key_t *range_key, const vtss_appl_ipmc_lib_profile_rule_conf_t *rule_conf, const vtss_appl_ipmc_lib_profile_range_key_t *insert_before_range_key)
{
    ipmc_lib_profile_itr_t      profile_itr;
    ipmc_lib_profile_rule_itr_t rule_itr, insert_before_rule_itr, next_rule_itr;
    ipmc_lib_profile_rule_t     rule;

    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(profile_key));
    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(range_key));
    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(rule_conf));

    T_DG(IPMC_LIB_TRACE_GRP_PROFILE, "profile_key = %s, range_key = %s, rule_conf.deny = %d, rule_conf.log = %d, insert_before_range_key = %s", profile_key->name, range_key->name, rule_conf->deny, rule_conf->log, insert_before_range_key ? insert_before_range_key->name : "nullptr");

    IPMC_LIB_PROFILE_LOCK_SCOPE();

    if ((profile_itr = IPMC_LIB_PROFILE_map.find(*profile_key)) == IPMC_LIB_PROFILE_map.end()) {
        return VTSS_APPL_IPMC_LIB_RC_PROFILE_NO_SUCH;
    }

    if (IPMC_LIB_PROFILE_range_map.find(*range_key) == IPMC_LIB_PROFILE_range_map.end()) {
        return VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_NO_SUCH;
    }

    for (rule_itr = profile_itr->second.rule_list.begin(); rule_itr != profile_itr->second.rule_list.end(); ++rule_itr) {
        if (rule_itr->range == *range_key) { // Using operator==()
            break;
        }
    }

    if (insert_before_range_key) {
        // Check if it should stay where it is or get craeted just before
        // another rule.
        if (strlen(insert_before_range_key->name) == 0) {
            // If insert_before_range_key->name[0] == '\0', we keep the rule
            // just where it is now. In this case the rule must already exist.
            if (rule_itr == profile_itr->second.rule_list.end()) {
                return VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_RULE_NO_SUCH;
            }

            // It is perfectly OK to have insert_before_rule_itr point to end().
            insert_before_rule_itr = rule_itr;
            ++insert_before_rule_itr;
        } else {
            // See if this range key exists in the rule list
            for (insert_before_rule_itr = profile_itr->second.rule_list.begin(); insert_before_rule_itr != profile_itr->second.rule_list.end(); ++insert_before_rule_itr) {
                if (insert_before_rule_itr->range == *insert_before_range_key) {
                    break;
                }
            }

            if (insert_before_rule_itr == profile_itr->second.rule_list.end()) {
                // Insert-after range key not found.
                return VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_INSERT_BEFORE_NO_SUCH;
            }
        }
    } else {
        // A nullptr means: Move or insert this rule at the end of the list. We
        // can do that by using the end of the list as insertion point.
        insert_before_rule_itr = profile_itr->second.rule_list.end();
    }

    rule.range = *range_key;
    rule.conf  = *rule_conf;

    if (rule_itr == profile_itr->second.rule_list.end()) {
        // This rule doesn't exist in list. Insert it.
        if (!profile_itr->second.rule_list.insert(insert_before_rule_itr, rule)) {
            return VTSS_APPL_IPMC_LIB_RC_OUT_OF_MEMORY;
        }
    } else {
        // Rule already exists in list. Move and/or update it.
        next_rule_itr = rule_itr;
        ++next_rule_itr;

        if (next_rule_itr == insert_before_rule_itr) {
            // Rule already at its insertion point. See if contents is changed.
            if (rule_itr->conf == *rule_conf) {
                // No changes.
                return VTSS_RC_OK;
            }

            rule_itr->conf = *rule_conf;
        } else {
            // Unfortunately, the vtss::List doesn't have a move() method, so we
            // need to erase it and create it again.
            profile_itr->second.rule_list.erase(rule_itr);

            if (!profile_itr->second.rule_list.insert(insert_before_rule_itr, rule)) {
                return VTSS_APPL_IPMC_LIB_RC_OUT_OF_MEMORY;
            }
        }
    }

    // If we get here, the profile has changed
    IPMC_LIB_PROFILE_notif_set(&profile_itr->first);

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_profile_rule_conf_del()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_profile_rule_conf_del(const vtss_appl_ipmc_lib_profile_key_t *profile_key, const vtss_appl_ipmc_lib_profile_range_key_t *range_key)
{
    ipmc_lib_profile_itr_t      profile_itr;
    ipmc_lib_profile_rule_itr_t rule_itr;

    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(profile_key));
    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(range_key));

    T_DG(IPMC_LIB_TRACE_GRP_PROFILE, "profile_key = %s, range_key = %s", profile_key->name, range_key->name);

    IPMC_LIB_PROFILE_LOCK_SCOPE();

    if ((profile_itr = IPMC_LIB_PROFILE_map.find(*profile_key)) == IPMC_LIB_PROFILE_map.end()) {
        return VTSS_APPL_IPMC_LIB_RC_PROFILE_NO_SUCH;
    }

    for (rule_itr = profile_itr->second.rule_list.begin(); rule_itr != profile_itr->second.rule_list.end(); ++rule_itr) {
        if (rule_itr->range == *range_key) { // Using operator==()
            break;
        }
    }

    if (rule_itr == profile_itr->second.rule_list.end()) {
        return VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_RULE_NO_SUCH;
    }

    profile_itr->second.rule_list.erase(rule_itr);

    // Notify observers of a change in the profile
    IPMC_LIB_PROFILE_notif_set(&profile_itr->first);

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_profile_rule_itr()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_profile_rule_itr(const vtss_appl_ipmc_lib_profile_key_t       *profile_prev, vtss_appl_ipmc_lib_profile_key_t       *profile_next,
                                            const vtss_appl_ipmc_lib_profile_range_key_t *range_prev,   vtss_appl_ipmc_lib_profile_range_key_t *range_next, bool stay_in_this_profile)
{
    ipmc_lib_profile_itr_t                 profile_itr;
    ipmc_lib_profile_rule_itr_t            rule_itr;
    vtss_appl_ipmc_lib_profile_key_t       prf_key;
    vtss_appl_ipmc_lib_profile_range_key_t rng_key;

    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(profile_next));
    VTSS_RC(IPMC_LIB_PROFILE_ptr_check(range_next));

    if (profile_prev) {
        prf_key = *profile_prev;
    } else {
        vtss_clear(prf_key);
    }

    if (range_prev) {
        rng_key = *range_prev;
    } else {
        vtss_clear(rng_key);
    }

    IPMC_LIB_PROFILE_LOCK_SCOPE();

    for (profile_itr = IPMC_LIB_PROFILE_map.greater_than_or_equal(prf_key); profile_itr != IPMC_LIB_PROFILE_map.end(); ++profile_itr) {
        if (profile_itr->first != prf_key) {
            if (stay_in_this_profile) {
                // We have been asked not to enter the next profile. Done
                // iterating.
                return VTSS_RC_ERROR;
            }

            // Start over
            vtss_clear(rng_key);
        }

        if (rng_key.name[0] == '\0') {
            // Get first
            rule_itr = profile_itr->second.rule_list.begin();
        } else {
            for (rule_itr = profile_itr->second.rule_list.begin(); rule_itr != profile_itr->second.rule_list.end(); ++rule_itr) {
                if (rule_itr->range == rng_key) {
                    ++rule_itr;
                    break;
                }
            }
        }

        if (rule_itr != profile_itr->second.rule_list.end()) {
            *profile_next = profile_itr->first;
            *range_next   = rule_itr->range;
            return VTSS_RC_OK;
        }
    }

    return VTSS_RC_ERROR;
}

/******************************************************************************/
// ipmc_lib_profile_init()
// Invoked once under INIT_CMD_INIT
/******************************************************************************/
mesa_rc ipmc_lib_profile_init(vtss_init_data_t *data)
{
    switch (data->cmd) {
    case INIT_CMD_INIT:
        critd_init(&IPMC_LIB_PROFILE_crit, "ipmc_profile", VTSS_MODULE_ID_IPMC_LIB, CRITD_TYPE_MUTEX);

        IPMC_LIB_PROFILE_capabilities_set();
        IPMC_LIB_PROFILE_global_default_conf_set();
        IPMC_LIB_PROFILE_global_conf = IPMC_LIB_PROFILE_global_default_conf;

        // Create an artificial profile element for notifications if profiles
        // are globally enabled/disabled. The reason we need this is that if no
        // profiles are enabled, we would not be able to make a notification
        // otherwise.
        // We use the - otherwise - reserved name "all" for this.
        strcpy(IPMC_LIB_PROFILE_key_global.name, "all");
        IPMC_LIB_PROFILE_notif_add(&IPMC_LIB_PROFILE_key_global);
        break;

    case INIT_CMD_CONF_DEF:
        if (data->isid == VTSS_ISID_GLOBAL) {
            IPMC_LIB_PROFILE_conf_default();
        }

    default:
        break;
    }

    return VTSS_RC_OK;
}

