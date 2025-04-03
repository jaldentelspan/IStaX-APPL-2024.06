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

#include "critd_api.h"                 /* For semaphore wrapper           */
#include <vtss/appl/psec.h>            /* For ourselves                   */
#include <psec_limit_api.h>            /* For ourselves                   */
#include <psec_util.h>                 /* For psec_util_mac_type_to_str() */
#include "msg_api.h"                   /* For msg_switch_is_primary()     */
#include "main.h"                      /* For vtss_xstr()                 */
#include "misc_api.h"                  /* For iport2uport()               */
#include "port_api.h"                  /* For port_count_max()            */
#include "vlan_api.h"                  /* For vlan_mgmt_port_conf_get()   */
#include <vtss/basics/map.hxx>
#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"                /* For S_xxx() macros              */
#endif
#include "psec_limit_trace.h"
#ifdef VTSS_SW_OPTION_ICFG
#include "psec_limit_icli_functions.h" /* For psec_limit_icfg_init()      */
#endif
#if defined(VTSS_SW_OPTION_SNMP)
#include "vtss_os_wrapper_snmp.h"
#endif

/****************************************************************************/
// Trace definitions
/****************************************************************************/

/* Trace registration. Initialized by psec_init() */
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "psec_limit", "Port Security Limit Control module"
};

#ifndef PSEC_LIMIT_DEFAULT_TRACE_LVL
#define PSEC_LIMIT_DEFAULT_TRACE_LVL VTSS_TRACE_LVL_WARNING
#endif

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        PSEC_LIMIT_DEFAULT_TRACE_LVL
    },
    [TRACE_GRP_ICLI] = {
        "icli",
        "ICLI",
        PSEC_LIMIT_DEFAULT_TRACE_LVL
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/******************************************************************************/
// Mutex stuff.
/******************************************************************************/
static critd_t PSEC_LIMIT_crit;

// Macros for accessing mutex functions
// -----------------------------------------
#define PSEC_LIMIT_CRIT_ENTER()         critd_enter(        &PSEC_LIMIT_crit, __FILE__, __LINE__)
#define PSEC_LIMIT_CRIT_EXIT()          critd_exit(         &PSEC_LIMIT_crit, __FILE__, __LINE__)
#define PSEC_LIMIT_CRIT_ASSERT_LOCKED() critd_assert_locked(&PSEC_LIMIT_crit, __FILE__, __LINE__)

/**
  * \brief Per-switch Port Security Limit Control Configuration.
  */
typedef struct  {
    /**
      * \brief Array of port configurations - one per front port on the switch.
      *
      * Index 0 is the first port.
      */
    CapArray<vtss_appl_psec_interface_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_conf;
} psec_limit_switch_conf_t;

// Overall configuration (valid on primary switch only).
typedef struct {
    // One instance of the global configuration
    vtss_appl_psec_global_conf_t global_conf;

    // One instance per port in the stack of the per-port configuration.
    // Index 0 corresponds to VTSS_ISID_START. Used by the primary switch
    // for the per-switch configuration.
    psec_limit_switch_conf_t switch_conf[VTSS_ISID_CNT];
} psec_limit_stack_conf_t;

static psec_limit_stack_conf_t PSEC_LIMIT_stack_conf;

/**
 * Array of Port VLAN IDs needed when converting a non-specified VLAN on a
 * statically added MAC address to a valid VLAN.
 */
static CapArray<mesa_vid_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> PSEC_LIMIT_pvids;

// Reference counting per port.
// Index 0 holds number of MACs that we told the PSEC module to put in forwarding state.
// Index 1 holds number of MACs that we told the PSEC module to put in blocked state.
static u32 PSEC_LIMIT_ref_cnt[VTSS_ISID_CNT][VTSS_MAX_PORTS_LEGACY_CONSTANT_USE_CAPARRAY_INSTEAD][2];

/**
 * \brief Key to use when looking up entries in the MAC status
 * (psec_limit_mac_status_t). See also "operator<" for this structure.
 */
typedef struct {
    vtss_ifindex_t ifindex;
    mesa_vid_t     vlan;
    mesa_mac_t     mac;
} psec_limit_mac_map_key_t;

static  vtss::Map<psec_limit_mac_map_key_t, vtss_appl_psec_mac_conf_t> psec_limit_mac_map;
typedef vtss::Map<psec_limit_mac_map_key_t, vtss_appl_psec_mac_conf_t>::iterator psec_limit_mac_itr_t;
typedef vtss::Map<psec_limit_mac_map_key_t, vtss_appl_psec_mac_conf_t>::const_iterator psec_limit_mac_const_itr_t;

/**
 * \brief Structure holding both ifindex, isid, and port number
 *
 * Annoyingly enough we use both <isid, port> and ifindex in this module, so
 * in order to avoid conversion all the time, we pass just this structure around.
 */
typedef struct {
    vtss_isid_t    isid;
    mesa_port_no_t port;
    vtss_ifindex_t ifindex;
} psec_limit_ifindex_t;

// Hold back our config until an INIT_CMD_ICFG_LOADING_POST event is seen.
static BOOL PSEC_LIMIT_icfg_loading_post_seen;

// In case of shared VLAN learning, we need to learn entries on a given FID, not
// a given VID. This is a cached version of the VLAN module's vlan_fid_table[].
static mesa_vid_t PSEC_LIMIT_vid_to_fid[VLAN_ENTRY_CNT];
#define PSEC_LIMIT_FID_GET(_vid_) ({                                           \
    mesa_vid_t _fid_ = PSEC_LIMIT_vid_to_fid[(_vid_) - VTSS_APPL_VLAN_ID_MIN]; \
    if (_fid_ == 0) {                                                          \
        _fid_ = _vid_;                                                         \
    }                                                                          \
                                                                               \
    _fid_; /* A.k.a. Statement Expression */                                   \
})

#define PSEC_LIMIT_FID_SET(_vid_, _fid_) PSEC_LIMIT_vid_to_fid[(_vid_) - VTSS_APPL_VLAN_ID_MIN] = (_fid_)

// If not debugging, set PSEC_LIMIT_INLINE to inline
#define PSEC_LIMIT_INLINE inline

// Dirty hack:
// Gotta get the PSEC module's mutex before our own so that we can control which
// MAC addresses to add first before getting learn MAC addresses from the port.
#define PSEC_LIMIT_MUTEX_ALL_ENTER() do {psec_mgmt_mutex_enter(); PSEC_LIMIT_CRIT_ENTER();} while (0)
#define PSEC_LIMIT_MUTEX_ALL_EXIT()  do {PSEC_LIMIT_CRIT_EXIT();  psec_mgmt_mutex_exit();}  while (0)

/******************************************************************************/
//
// Module Private Functions
//
/******************************************************************************/

#define PSEC_LIMIT_MAC_STR_BUF_SIZE 70

/******************************************************************************/
// PSEC_LIMIT_mac_conf_to_str()
/******************************************************************************/
static const char *PSEC_LIMIT_mac_conf_to_str(vtss_ifindex_t ifindex, const vtss_appl_psec_mac_conf_t *const mac_conf, char buf[PSEC_LIMIT_MAC_STR_BUF_SIZE])
{
    char mac_str[18];
    sprintf(buf, "<Interface, VID, MAC> = <%u, %u, %s>", VTSS_IFINDEX_PRINTF_ARG(ifindex), mac_conf->vlan, misc_mac_txt(mac_conf->mac.addr, mac_str));
    return buf;
}

/******************************************************************************/
// PSEC_LIMIT_mac_itr_to_str()
/******************************************************************************/
static const char *PSEC_LIMIT_mac_itr_to_str(psec_limit_mac_itr_t mac_itr, char buf[PSEC_LIMIT_MAC_STR_BUF_SIZE])
{
    char mac_str[18];
    sprintf(buf, "<Interface, VID, MAC> = <%u, %u, %s>", VTSS_IFINDEX_PRINTF_ARG(mac_itr->first.ifindex), mac_itr->second.vlan, misc_mac_txt(mac_itr->second.mac.addr, mac_str));
    return buf;
}


/******************************************************************************/
// psec_limit_mac_map_key_t::operator<
// Used for sorting when inserting entries into the map.
/******************************************************************************/
bool operator<(const psec_limit_mac_map_key_t &lhs, const psec_limit_mac_map_key_t &rhs)
{
    // First sort by ifindex
    if (lhs.ifindex != rhs.ifindex) {
        return lhs.ifindex < rhs.ifindex;
    }

    // Then sort by VLAN
    if (lhs.vlan != rhs.vlan) {
        return lhs.vlan < rhs.vlan;
    }

    // Finally by MAC address
    return memcmp(lhs.mac.addr, rhs.mac.addr, sizeof(lhs.mac.addr)) < 0;
}

/******************************************************************************/
// PSEC_LIMIT_mac_itr_get_from_mac_conf()
// Use this function to get an iterator that matches VLAN ID and MAC, but
// disregarding ifindex.
/******************************************************************************/
static psec_limit_mac_itr_t PSEC_LIMIT_mac_itr_get_from_mac_conf(const vtss_appl_psec_mac_conf_t *const entry)
{
    psec_limit_mac_itr_t mac_itr = psec_limit_mac_map.begin();

    for (mac_itr = psec_limit_mac_map.begin(); mac_itr != psec_limit_mac_map.end(); ++mac_itr) {
        if (mac_itr->first.vlan == entry->vlan && memcmp(&mac_itr->first.mac, &entry->mac, sizeof(mac_itr->first.mac)) == 0) {
            return mac_itr;
        }
    }

    return psec_limit_mac_map.end();
}

/******************************************************************************/
// PSEC_LIMIT_mac_itr_get()
/******************************************************************************/
static psec_limit_mac_itr_t PSEC_LIMIT_mac_itr_get(const psec_limit_mac_map_key_t *key)
{
    return psec_limit_mac_map.find(*key);
}

/******************************************************************************/
// PSEC_LIMIT_mac_itr_get_first_from_ifindex()
/******************************************************************************/
static psec_limit_mac_itr_t PSEC_LIMIT_mac_itr_get_first_from_ifindex(vtss_ifindex_t ifindex)
{
    psec_limit_mac_itr_t mac_itr;

    for (mac_itr = psec_limit_mac_map.begin(); mac_itr != psec_limit_mac_map.end(); ++mac_itr) {
        if (mac_itr->first.ifindex == ifindex) {
            return mac_itr;
        } else if (mac_itr->first.ifindex > ifindex) {
            // No need to look further because the map is sorted by ifindex as first key.
            break;
        }
    }

    return psec_limit_mac_map.end();
}

/******************************************************************************/
// PSEC_LIMIT_mac_itr_get_next_from_ifindex()
// Returns an iterator to a MAC entry that has an ifindex which is higher than
// #ifindex
/******************************************************************************/
static psec_limit_mac_itr_t PSEC_LIMIT_mac_itr_get_next_from_ifindex(vtss_ifindex_t ifindex)
{
    psec_limit_mac_itr_t mac_itr;

    for (mac_itr = psec_limit_mac_map.begin(); mac_itr != psec_limit_mac_map.end(); ++mac_itr) {
        if (mac_itr->first.ifindex > ifindex) {
            return mac_itr;
        }
    }

    return psec_limit_mac_map.end();
}

/******************************************************************************/
// PSEC_LIMIT_mac_itr_get_from_fid_mac()
/******************************************************************************/
static psec_limit_mac_itr_t PSEC_LIMIT_mac_itr_get_from_fid_mac(mesa_vid_t fid, const mesa_mac_t *mac)
{
    psec_limit_mac_itr_t mac_itr;

    for (mac_itr = psec_limit_mac_map.begin(); mac_itr != psec_limit_mac_map.end(); ++mac_itr) {
        if (PSEC_LIMIT_FID_GET(mac_itr->first.vlan) == fid && memcmp(&mac_itr->first.mac, mac, sizeof(mac_itr->first.mac)) == 0) {
            return mac_itr;
        }
    }

    return psec_limit_mac_map.end();
}

/******************************************************************************/
// PSEC_LIMIT_mac_itr_get_first()
/******************************************************************************/
static psec_limit_mac_itr_t PSEC_LIMIT_mac_itr_get_first(void)
{
    return psec_limit_mac_map.begin();
}

/******************************************************************************/
// PSEC_LIMIT_ifindex_from_port()
/******************************************************************************/
static mesa_rc PSEC_LIMIT_ifindex_from_port(vtss_isid_t isid, mesa_port_no_t port, psec_limit_ifindex_t *psec_limit_ifindex, int line_no)
{
    if (vtss_ifindex_from_port(isid, port, &psec_limit_ifindex->ifindex) != VTSS_RC_OK) {
        T_E("Line %d: Unable to convert <isid, port> = <%u, %u> to ifindex", line_no, isid, port);
        // Our return value is better than vtss_ifindex_from_port()'s.
        return VTSS_APPL_PSEC_RC_INV_PORT;
    }

    psec_limit_ifindex->isid = isid;
    psec_limit_ifindex->port = port;

    return VTSS_RC_OK;
}

/******************************************************************************/
// PSEC_LIMIT_ifindex_from_ifindex()
/******************************************************************************/
static mesa_rc PSEC_LIMIT_ifindex_from_ifindex(vtss_ifindex_t ifindex, psec_limit_ifindex_t *psec_limit_ifindex, int line_no, BOOL give_error = TRUE)
{
    vtss_ifindex_elm_t ife;

    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK) {
        if (give_error) {
            T_E("Line %d: Unable to decompose ifindex = %u", line_no, VTSS_IFINDEX_PRINTF_ARG(ifindex));
        }

        return PSEC_LIMIT_ERROR_INV_IFINDEX;
    }

    if (ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
        if (give_error) {
            T_E("Line %d: ifindex = %u is not a port type", line_no, VTSS_IFINDEX_PRINTF_ARG(ifindex));
        }

        return PSEC_LIMIT_ERROR_IFINDEX_NOT_REPRESENTING_A_PORT;
    }

    psec_limit_ifindex->isid = ife.isid;
    psec_limit_ifindex->port = ife.ordinal;
    psec_limit_ifindex->ifindex = ifindex;

    return VTSS_RC_OK;
}

/******************************************************************************/
// PSEC_LIMIT_primary_switch_isid_port_check()
// Returns VTSS_RC_OK if we're the primary switch and isid and port are legal.
/******************************************************************************/
static mesa_rc PSEC_LIMIT_primary_switch_isid_port_check(vtss_isid_t isid, mesa_port_no_t port, BOOL check_port)
{
    if (!msg_switch_is_primary()) {
        return PSEC_LIMIT_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    if (!VTSS_ISID_LEGAL(isid)) {
        return PSEC_LIMIT_ERROR_INV_ISID;
    }

    if (check_port && port >= port_count_max()) {
        // port is only set to something different from 0 in case we really need
        // to check that the port exists on a given switch and not a stack port,
        // so it's safe to check against actual number of ports rather than
        // checking against fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT).
        return PSEC_LIMIT_ERROR_INV_PORT;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// PSEC_LIMIT_mac_itr_alloc()
/******************************************************************************/
static psec_limit_mac_itr_t PSEC_LIMIT_mac_itr_alloc(vtss_ifindex_t ifindex, const vtss_appl_psec_mac_conf_t *const mac_conf)
{
    psec_limit_mac_itr_t     mac_itr;
    psec_limit_mac_map_key_t key;
    char                     buf[PSEC_LIMIT_MAC_STR_BUF_SIZE];

    key.ifindex = ifindex;
    key.vlan    = mac_conf->vlan;
    key.mac     = mac_conf->mac;

    // The .get() method allocates if it can't find an existing (which it shouldn't
    // be able to, given that we are about to allocate a new).
    mac_itr = psec_limit_mac_map.get(key);
    if (mac_itr != psec_limit_mac_map.end()) {
        mac_itr->second = *mac_conf;
    }

    T_I("%s entry for %s", mac_itr == psec_limit_mac_map.end() ? "Failed to create" : "Successfully created", PSEC_LIMIT_mac_conf_to_str(ifindex, mac_conf, buf));

    return mac_itr;
}

/******************************************************************************/
// PSEC_LIMIT_sticky_add()
// Invoked when we're in sticky mode and a new MAC address arrives or when we go
// into sticky mode and convert existing dynamic entries into sticky.
/******************************************************************************/
static void PSEC_LIMIT_sticky_add(vtss_ifindex_t ifindex, vtss_appl_psec_mac_conf_t *mac_conf)
{
    psec_limit_ifindex_t psec_limit_ifindex;
    psec_limit_mac_itr_t mac_itr;
    char                 buf[PSEC_LIMIT_MAC_STR_BUF_SIZE];
    BOOL                 add = FALSE;

    (void)PSEC_LIMIT_ifindex_from_ifindex(ifindex, &psec_limit_ifindex, __LINE__);
    (void)PSEC_LIMIT_mac_conf_to_str(ifindex, mac_conf, buf);
    T_I("Considering %s", buf);

    // Gotta add this new MAC address to our map if not already there.
    mac_conf->mac_type = VTSS_APPL_PSEC_MAC_TYPE_STICKY;

    if ((mac_itr = PSEC_LIMIT_mac_itr_get_from_fid_mac(PSEC_LIMIT_FID_GET(mac_conf->vlan), &mac_conf->mac)) != psec_limit_mac_map.end()) {
        T_I("%s: Already Found", buf);

        if (ifindex != mac_itr->first.ifindex || mac_conf->vlan != mac_itr->first.vlan) {
            // We already have this entry either on another port or another VLAN
            // mapping to the same FID.
            psec_limit_ifindex_t            other_psec_limit_ifindex;
            vtss_appl_psec_interface_conf_t *other_port_conf;

            if (PSEC_LIMIT_ifindex_from_ifindex(mac_itr->first.ifindex, &other_psec_limit_ifindex, __LINE__) != VTSS_RC_OK) {
                // There's a bug that must be fixed
                return;
            }

            other_port_conf = &PSEC_LIMIT_stack_conf.switch_conf[other_psec_limit_ifindex.isid - VTSS_ISID_START].port_conf[other_psec_limit_ifindex.port];

            // Seems we already have an entry for this one on either another
            // port or another VLAN mapping to the same FID.
            if (ifindex != mac_itr->first.ifindex) {
                // It's on another port - and herhaps also on another VLAN.
                // If that other interface is PSEC_LIMIT-enabled, it's an error,
                // because the PSEC module should detect MAC moves of static
                // entries.
                // If that other interface is not enabled, it's not an error,
                // but we will remove the one from the other interface.
                if (other_port_conf->enabled) {
                    T_E("%s: Already found on ifindex = %u", buf, VTSS_IFINDEX_PRINTF_ARG(other_psec_limit_ifindex.ifindex));
                } else {
                    T_I("%s: Already found on ifindex = %u, but will be moved to new interface, because that old is disabled", buf, VTSS_IFINDEX_PRINTF_ARG(other_psec_limit_ifindex.ifindex));
                    psec_limit_mac_map.erase(mac_itr);
                    add = TRUE;
                }
            } else {
                // It's on the same port, but on another VLAN mapping to the
                // same FID. This should have been detected by the PSEC module
                // before it got a chance to get added.
                T_E("%s: Already found on VLAN %u, which maps to the same FID as this one", buf, mac_itr->first.vlan);
            }
        } else {
            T_I("%s: Already in our map on this port");
        }
    } else {
        // We are not aware of this MAC.
        T_I("%s: We are not aware of this MAC", buf);
        add  = TRUE;
    }

    if (add) {
        T_I("%s: Adding it as sticky", buf);
        if (PSEC_LIMIT_mac_itr_alloc(ifindex, mac_conf) == psec_limit_mac_map.end()) {
            char mac_str[18];
            // Out of memory?
            (void)misc_mac_txt(mac_conf->mac.addr, mac_str);
            S_PORT_W(psec_limit_ifindex.isid, psec_limit_ifindex.port, "PORT-SECURITY: Unable to add sticky <VID, MAC> = <%u, %s> to MAC map", mac_conf->vlan, mac_str);
            T_W("PORT-SECURITY: Unable to add sticky <VID, MAC> = <%u, %s> to MAC map on port %u", mac_conf->vlan, mac_str, iport2uport(psec_limit_ifindex.port));
        }
    }
}

/******************************************************************************/
// PSEC_LIMIT_on_mac_add_callback()
// This function will be called by the PSEC module whenever a new MAC address
// is to be added.
/******************************************************************************/
static psec_add_method_t PSEC_LIMIT_on_mac_add_callback(vtss_isid_t isid, mesa_port_no_t port, mesa_vid_mac_t *vid_mac, u32 mac_cnt_before_callback, vtss_appl_psec_user_t originating_user, psec_add_action_t *action)
{
    mesa_rc                         rc;
    vtss_appl_psec_interface_conf_t *port_conf;
    psec_add_method_t               result = PSEC_ADD_METHOD_FORWARD;
    u32                             *fwd_cnt, *blk_cnt;

#ifdef VTSS_SW_OPTION_SYSLOG
    char prefix_str[150];
    char mac_str[18];

    (void)misc_mac_txt(vid_mac->mac.addr, mac_str);
    sprintf(prefix_str, "PORT-SECURITY: Interface %s, MAC %s. ", SYSLOG_PORT_INFO_REPLACE_KEYWORD, mac_str);
#endif /* VTSS_SW_OPTION_SYSLOG */

    if ((rc = PSEC_LIMIT_primary_switch_isid_port_check(isid, port, TRUE)) != VTSS_RC_OK || action == NULL) {
        if (rc != PSEC_LIMIT_ERROR_MUST_BE_PRIMARY_SWITCH) {
            T_E("Internal error: Invalid parameter (rc = %d)", rc);
        }

        return PSEC_ADD_METHOD_FORWARD; // There's a bug that must be fixed, so we can return anything.
    }

    if (originating_user == VTSS_APPL_PSEC_USER_ADMIN) {
        // We ourselves are the reason for this call, so don't take the mutex
        PSEC_LIMIT_CRIT_ASSERT_LOCKED();
    } else {
        // Someone else caused this MAC address to be added. Take our own mutex
        PSEC_LIMIT_CRIT_ENTER();
    }

    port_conf = &PSEC_LIMIT_stack_conf.switch_conf[isid - VTSS_ISID_START].port_conf[port];
    if (!port_conf->enabled && originating_user != VTSS_APPL_PSEC_USER_ADMIN) {
        // Limit control is not enabled on that port and we are not about to
        // enable it. Allow it.
        goto do_exit;
    }

    // PSEC_LIMIT_ref_cnt[][][] uses zero-based indexing.
    fwd_cnt = &PSEC_LIMIT_ref_cnt[isid - VTSS_ISID_START][port][0];
    blk_cnt = &PSEC_LIMIT_ref_cnt[isid - VTSS_ISID_START][port][1];

    T_I("%u:%u: Enter ADD MAC. Cur Fwd = %u, Cur Blk = %u", isid, iport2uport(port), *fwd_cnt, *blk_cnt);

    if (*fwd_cnt + *blk_cnt != mac_cnt_before_callback) {
        // This may occur because - from a PSEC module perspective - the "on-mac-add" call is not protected by PSEC module's mutex, whereas the "on-mac-del" call is.
        T_I("%u:%u: Disagreement between PSEC and PSEC Limit: mac_cnt = %u, fwd = %u, blk = %u", isid, iport2uport(port), mac_cnt_before_callback, *fwd_cnt, *blk_cnt);
    }

    switch (port_conf->violation_mode) {
    case VTSS_APPL_PSEC_VIOLATION_MODE_PROTECT:

        // If violation_mode == PROTECT, then we shouldn't be called if the number of MAC addresses before this
        // one gets added is at or above the limit, since we've turned off CPU copy in previous call.
        if (mac_cnt_before_callback >= port_conf->limit) {
            T_E("%u:%u: Called with invalid mac_cnt (%u, %u, %u). Limit=%u, violation_mode=%d", isid, iport2uport(port), mac_cnt_before_callback, *fwd_cnt, *blk_cnt, port_conf->limit, port_conf->violation_mode);
        }

    // Fall through
    case VTSS_APPL_PSEC_VIOLATION_MODE_RESTRICT:
        // In protect and restrict mode, we must always return limit reached as long as the number of
        // forwarding MAC addresses is at the limit.
        *action = *fwd_cnt + 1 >= port_conf->limit ? PSEC_ADD_ACTION_LIMIT_REACHED : PSEC_ADD_ACTION_NONE;

#ifdef VTSS_SW_OPTION_SYSLOG
        // Send a message to the syslog when reaching the limit.
        if (*fwd_cnt + 1 == port_conf->limit) {
            S_PORT_I(isid, port, "%sLimit reached (%s mode).", prefix_str, port_conf->violation_mode == VTSS_APPL_PSEC_VIOLATION_MODE_PROTECT ? "protect" : "restrict");
        }
#endif
        break;

    case VTSS_APPL_PSEC_VIOLATION_MODE_SHUTDOWN:
        // If violation_mode == SHUTDOWN, then we shouldn't be called if the number of MAC addresses before this
        // one gets added is above the limit, since we've shut down the port (and removed all MAC addresses)
        // prior to this call.
        if (mac_cnt_before_callback > port_conf->limit) {
            T_E("%u:%u: Called with invalid mac_cnt (%u, %u, %u). Limit=%u, violation_mode=%d", isid, iport2uport(port), mac_cnt_before_callback, *fwd_cnt, *blk_cnt, port_conf->limit, port_conf->violation_mode);
        }

        // When *reaching* the limit, set limit reached. The PSEC module will keep CPU-copying enabled
        // because we've set the port mode to PSEC_PORT_MODE_RESTRICT.
        // When *exceeding* the limit, shut down the port.
        *action = *fwd_cnt + 1 == port_conf->limit ? PSEC_ADD_ACTION_LIMIT_REACHED : *fwd_cnt == port_conf->limit ? PSEC_ADD_ACTION_SHUT_DOWN : PSEC_ADD_ACTION_NONE;

        // When shutting down the port, the forward decision doesn't really matter to the PSEC module,
        // but it doesn't harm to set it "correctly".
        result = *fwd_cnt < port_conf->limit ? PSEC_ADD_METHOD_FORWARD : PSEC_ADD_METHOD_BLOCK;

#ifdef VTSS_SW_OPTION_SYSLOG
        if (*action == PSEC_ADD_ACTION_SHUT_DOWN) {
            S_PORT_W(isid, port, "%sLimit exceeded. Shutting down the port.", prefix_str);
        }
#endif
        break;

    default:
        T_E("Unknown violation mode (%d)", port_conf->violation_mode);
        break;
    }

    // No matter the violation_mode, we allow this MAC address as long as we haven't reached the limit
    // and block it if limit is reached
    result = *fwd_cnt < port_conf->limit ? PSEC_ADD_METHOD_FORWARD : PSEC_ADD_METHOD_BLOCK;

    if (result == PSEC_ADD_METHOD_FORWARD) {
        (*fwd_cnt)++;

        if (port_conf->sticky) {
            // Add this MAC address to our map - if not already there.
            psec_limit_ifindex_t      psec_limit_ifindex;
            vtss_appl_psec_mac_conf_t mac_conf;

            (void)PSEC_LIMIT_ifindex_from_port(isid, port, &psec_limit_ifindex, __LINE__);
            mac_conf.vlan = vid_mac->vid;
            mac_conf.mac  = vid_mac->mac;
            PSEC_LIMIT_sticky_add(psec_limit_ifindex.ifindex, &mac_conf);
        }
    } else {
        (*blk_cnt)++;
    }

    T_D("%u:%u: ADD MAC. New Fwd = %u, New Blk = %u: %s - %s)",
        isid, iport2uport(port),
        *fwd_cnt, *blk_cnt,
        psec_add_method_to_str(result),
        *action == PSEC_ADD_ACTION_LIMIT_REACHED ? "Limit reached" : *action == PSEC_ADD_ACTION_SHUT_DOWN ? "Shutdown" : "Ready");

do_exit:
    if (originating_user != VTSS_APPL_PSEC_USER_ADMIN) {
        PSEC_LIMIT_CRIT_EXIT();
    }

    return result;
}

/******************************************************************************/
// PSEC_LIMIT_on_mac_del_callback()
/******************************************************************************/
static void PSEC_LIMIT_on_mac_del_callback(vtss_isid_t isid, mesa_port_no_t port, const mesa_vid_mac_t *vid_mac, psec_del_reason_t reason, psec_add_method_t add_method, vtss_appl_psec_user_t originating_user)
{
    int idx = add_method == PSEC_ADD_METHOD_FORWARD ? 0 : 1;
    u32 *val, *fwd_cnt, *blk_cnt;

    if (originating_user == VTSS_APPL_PSEC_USER_ADMIN) {
        // We ourselves are the reason for this call, so don't take the mutex
        PSEC_LIMIT_CRIT_ASSERT_LOCKED();
    } else {
        // Someone else caused this MAC address to be deleted. Take our own mutex
        PSEC_LIMIT_CRIT_ENTER();
    }

    // PSEC_LIMIT_ref_cnt[][][] uses zero-based indexing.
    fwd_cnt = &PSEC_LIMIT_ref_cnt[isid - VTSS_ISID_START][port][0];
    blk_cnt = &PSEC_LIMIT_ref_cnt[isid - VTSS_ISID_START][port][1];
    val     = &PSEC_LIMIT_ref_cnt[isid - VTSS_ISID_START][port][idx];

    T_I("%u:%u: Enter DEL MAC. Cur Fwd = %u, Cur Blk = %u", isid, iport2uport(port), *fwd_cnt, *blk_cnt);

    if (add_method != PSEC_ADD_METHOD_FORWARD && add_method != PSEC_ADD_METHOD_BLOCK) {
        // Odd to get called with an add_method that this module doesn't support.
        // We can only add with forward or block.
        T_E("%u:%u: Odd add_method %d", isid, iport2uport(port), add_method);
        goto do_exit;
    }

    if (*val == 0) {
        T_E("%u:%u: Reference count for add_method %s is 0", isid, iport2uport(port), psec_add_method_to_str(add_method));
        goto do_exit;
    }

    (*val)--;
    T_D("%u:%u: DEL MAC. New Fwd = %u, New Blk = %u (was added with Method = %s)", isid, iport2uport(port), *fwd_cnt, *blk_cnt, psec_add_method_to_str(add_method));

do_exit:
    if (originating_user != VTSS_APPL_PSEC_USER_ADMIN) {
        PSEC_LIMIT_CRIT_EXIT();
    }
}

/******************************************************************************/
// PSEC_LIMIT_conf_valid_port()
/******************************************************************************/
static mesa_rc PSEC_LIMIT_conf_valid_port(const vtss_appl_psec_interface_conf_t *const port_conf)
{
#if PSEC_LIMIT_MIN > 0
    if (port_conf->limit < PSEC_LIMIT_MIN) {
        return PSEC_LIMIT_ERROR_INV_LIMIT;
    }
#endif

    if (port_conf->limit > PSEC_LIMIT_MAX) {
        return PSEC_LIMIT_ERROR_INV_LIMIT;
    }

    if (port_conf->violate_limit < PSEC_VIOLATE_LIMIT_MIN || port_conf->violate_limit > PSEC_VIOLATE_LIMIT_MAX) {
        return PSEC_LIMIT_ERROR_INV_VIOLATE_LIMIT;
    }

    if (port_conf->violation_mode >= VTSS_APPL_PSEC_VIOLATION_MODE_LAST) {
        return PSEC_LIMIT_ERROR_INV_VIOLATION_MODE;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// PSEC_LIMIT_sticky_remove()
// Here, we have both the PSEC and the PSEC_LIMIT mutex.
/******************************************************************************/
static mesa_rc PSEC_LIMIT_sticky_remove(vtss_isid_t isid, mesa_port_no_t port)
{
    psec_limit_ifindex_t psec_limit_ifindex;
    psec_limit_mac_itr_t mac_itr;
    mesa_rc              rc;

    if ((rc = PSEC_LIMIT_ifindex_from_port(isid, port, &psec_limit_ifindex, __LINE__)) != VTSS_RC_OK) {
        // There's a bug that must be fixed
        return rc;
    }

    mac_itr = PSEC_LIMIT_mac_itr_get_first_from_ifindex(psec_limit_ifindex.ifindex);
    while (mac_itr != psec_limit_mac_map.end()) {
        psec_limit_mac_itr_t next_mac_itr = mac_itr;
        ++next_mac_itr;

        if (mac_itr->first.ifindex > psec_limit_ifindex.ifindex) {
            break;
        }

        if (mac_itr->second.mac_type == VTSS_APPL_PSEC_MAC_TYPE_STICKY) {
            psec_limit_mac_map.erase(mac_itr);
        }

        mac_itr = next_mac_itr;
    }

    return rc;
}

/******************************************************************************/
// PSEC_LIMIT_global_conf_apply()
/******************************************************************************/
static mesa_rc PSEC_LIMIT_global_conf_apply(const vtss_appl_psec_global_conf_t *new_conf)
{
    vtss_appl_psec_global_conf_t *old_conf = &PSEC_LIMIT_stack_conf.global_conf;
    mesa_rc                      rc;

    PSEC_LIMIT_CRIT_ASSERT_LOCKED();

    if (!PSEC_LIMIT_icfg_loading_post_seen) {
        if (new_conf) {
            // Cache it for later
            *old_conf = *new_conf;
        }

        return VTSS_RC_OK;
    }

    if (new_conf == NULL) {
        // Force-applying whatever we already had
        new_conf = old_conf;
    }

    // Change the age- and hold-times
    T_I("Invoking psec_mgmt_time_conf_set_special(enable aging = %d, aging period = %u, hold time = %u)", new_conf->enable_aging, new_conf->aging_period_secs, new_conf->hold_time_secs);
    if ((rc = psec_mgmt_time_conf_set_special(VTSS_APPL_PSEC_USER_ADMIN, new_conf->enable_aging ? new_conf->aging_period_secs : 0, new_conf->hold_time_secs)) != VTSS_RC_OK) {
        T_E("psec_mgmt_time_conf_set_special() failed: %s", error_txt(rc));
        return rc;
    }

    if (new_conf != old_conf) {
        *old_conf = *new_conf;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// PSEC_LIMIT_port_conf_apply()
// if new_conf is NULL; we force-apply the stored port conf
/******************************************************************************/
static mesa_rc PSEC_LIMIT_port_conf_apply(vtss_isid_t isid, mesa_port_no_t port, const vtss_appl_psec_interface_conf_t *new_conf)
{
    vtss_isid_t                     zisid = isid - VTSS_ISID_START;
    vtss_appl_psec_interface_conf_t *old_conf = &PSEC_LIMIT_stack_conf.switch_conf[zisid].port_conf[port], old_saved_conf;
    bool                            force_apply = new_conf == NULL;
    BOOL                            old_was_enabled, new_is_enabled;
    BOOL                            old_was_sticky, new_is_sticky;
    BOOL                            call_ena_func;
    BOOL                            reopen_port;
    BOOL                            limit_reached;
    mesa_rc                         rc;

    PSEC_LIMIT_CRIT_ASSERT_LOCKED();

    T_I("Enter");

    if (!msg_switch_exists(isid) || !PSEC_LIMIT_icfg_loading_post_seen) {
        if (new_conf) {
            // Cache it for later
            *old_conf = *new_conf;
        }

        return VTSS_RC_OK;
    }

    if (new_conf == NULL) {
        // Force-applying whatever we already had
        new_conf = old_conf;
    }

    // If new_conf is NULL (force_apply == TRUE), assume old_was_enabled == FALSE
    old_was_enabled = force_apply ? FALSE : old_conf->enabled;
    new_is_enabled  = new_conf->enabled;
    old_was_sticky  = old_conf->sticky;
    new_is_sticky   = new_conf->sticky;
    call_ena_func   = FALSE;
    reopen_port     = FALSE;
    limit_reached   = FALSE;

    if (!new_conf->sticky) {
        // Remove any entries that are sticky prior to changing mode below.
        rc = PSEC_LIMIT_sticky_remove(isid, port);
    }

    if (!old_was_enabled) {
        if (new_is_enabled) {
            // Old was not enabled, but we're going to enable.
            // Remove all entries from the port.
            call_ena_func = TRUE;
            reopen_port   = FALSE;
            limit_reached = new_conf->limit == 0;
        }
    } else {
        if (!new_is_enabled) {
            // We're going from enabled to disabled.
            // Delete limits and shutdown properties on the port.
            call_ena_func = TRUE;
            reopen_port   = TRUE;
        } else {
            // Old was enabled and new is enabled.
            // According to DS, we should remove all entries on the port if the limit or the violation_mode changes
            // (a bit silly I think, but they get what they want).
            if (new_conf->limit          != old_conf->limit          ||
                new_conf->violation_mode != old_conf->violation_mode ||
                new_conf->violate_limit  != old_conf->violate_limit) {
                call_ena_func = TRUE;
                reopen_port   = TRUE;
                limit_reached = new_conf->limit == 0;
            }
        }
    }

    old_saved_conf = *old_conf;
    if (new_conf != old_conf) {
        // Save configuration
        *old_conf = *new_conf;
    }

    if (call_ena_func) {
        // Both when violation_mode is VTSS_APPL_PSEC_VIOLATION_MODE_RESTRICT and VTSS_APPL_PSEC_VIOLATION_MODE_SHUTDOWN,
        // we ask for keeping CPU copying enabled even though the limit is reached. In the first case
        // because we need to count violating MAC addresses, and in the second case, because we
        // need an extra MAC address beyond the limit to trigger the shut down.
        psec_port_mode_t port_mode = new_conf->violation_mode == VTSS_APPL_PSEC_VIOLATION_MODE_PROTECT ? PSEC_PORT_MODE_NORMAL : PSEC_PORT_MODE_RESTRICT;

        PSEC_LIMIT_ref_cnt[zisid][port][0] = 0;
        PSEC_LIMIT_ref_cnt[zisid][port][1] = 0;

        // This function may call psec_limit_mgmt_static_macs_get().
        T_I("Invoking psec_mgmt_port_conf_set_special())");
        rc = psec_mgmt_port_conf_set_special(VTSS_APPL_PSEC_USER_ADMIN, isid, port, new_is_enabled, port_mode, reopen_port, limit_reached, new_conf->violate_limit, new_is_enabled ? new_conf->sticky : FALSE);

        if (rc != VTSS_RC_OK) {
            T_E("psec_mgmt_port_conf_set_special() failed: %s", error_txt(rc));
            goto exit_with_err;
        }
    } else if (new_is_enabled && new_is_sticky != old_was_sticky) {
        // Above, we passed the sticky flag in the call to
        // psec_mgmt_port_conf_set_special(), but that function is not
        // called when only the sticky flag changes (and we can't call
        // it because it will also delete all MACs when enabling
        // ourselves. If we enable the sticky flag and we are PSEC-
        // enabled, we must also convert all dynamic entries to sticky
        // entries. Likewise, we we disable the sticky flag, we must
        // ask PSEC to change all its sticky entries to dynamic entries.
        psec_limit_ifindex_t psec_limit_ifindex;
        VTSS_RC(PSEC_LIMIT_ifindex_from_port(isid, port, &psec_limit_ifindex, __LINE__));
        T_I("Invoking psec_mgmt_port_sticky_set(). New sticky = %d)", new_conf->sticky);
        if ((rc = psec_mgmt_port_sticky_set(psec_limit_ifindex.ifindex, new_conf->sticky, new_conf->sticky ? PSEC_LIMIT_sticky_add : NULL)) != VTSS_RC_OK) {
            T_E("psec_mgmt_port_sticky_set() failed: %s", error_txt(rc));
            goto exit_with_err;
        }
    }

    return VTSS_RC_OK;

exit_with_err:
    *old_conf = old_saved_conf;
    return rc;

}

/******************************************************************************/
// PSEC_LIMIT_mac_cnt()
/******************************************************************************/
static u32 PSEC_LIMIT_mac_cnt(vtss_ifindex_t ifindex)
{
    psec_limit_mac_itr_t mac_itr;
    u32                  mac_cnt = 0;

    for (mac_itr = psec_limit_mac_map.begin(); mac_itr != psec_limit_mac_map.end(); ++mac_itr) {
        if (mac_itr->first.ifindex == ifindex) {
            mac_cnt++;
        } else if (mac_itr->first.ifindex > ifindex) {
            // No need to look further because the map is sorted by ifindex as first key.
            break;
        }
    }

    return mac_cnt;
}

/******************************************************************************/
// PSEC_LIMIT_check_limit()
/******************************************************************************/
static mesa_rc PSEC_LIMIT_check_limit(psec_limit_ifindex_t *psec_limit_ifindex, BOOL enabled, u32 limit, BOOL adding_mac)
{
    u32 cur_cnt;

    if (enabled) {
        // Get number of forwarding entries on this port
        cur_cnt = PSEC_LIMIT_ref_cnt[psec_limit_ifindex->isid - VTSS_ISID_START][psec_limit_ifindex->port][0];
    } else {
        cur_cnt = PSEC_LIMIT_mac_cnt(psec_limit_ifindex->ifindex);
    }

    if (adding_mac) {
        if (cur_cnt >= limit) {
            // Not room for more MAC addresses on this interface.
            return VTSS_APPL_PSEC_RC_LIMIT_IS_REACHED;
        }
    } else {
        // Changing limit
        if (limit < cur_cnt) {
            return PSEC_LIMIT_ERROR_LIMIT_LOWER_THAN_CUR_CNT;
        }
    }

    // Even though the limit is not reached on this port, it could be that the
    // grand total of sticky/static entries amounts to the total number of MAC
    // addresses manageable by PSEC.
    if (adding_mac && psec_limit_mac_map.size() >= PSEC_MAC_ADDR_ENTRY_CNT) {
        return PSEC_LIMIT_ERROR_STATIC_STICKY_MAC_CNT_EXCEEDS_GLOBAL_POOL_SIZE;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// PSEC_LIMIT_default()
// Create defaults for either
//   1) the global section (#default_all == FALSE && isid == VTSS_ISID_GLOBAL)
//   2) one switch (#default_all == FALSE && VTSS_ISID_LEGAL(isid), or for both
//   3) for both the global section and all switches (#default_all == TRUE).
/******************************************************************************/
static void PSEC_LIMIT_default(vtss_isid_t isid, BOOL default_all)
{
    T_D("Enter, isid: %d, default_all = %d", isid, default_all);

    // Gotta get the PSEC module's mutex first so that it doesn't add new MAC
    // addresses behind our back - and in order to avoid deadlocks.
    PSEC_LIMIT_MUTEX_ALL_ENTER();

    if (default_all || isid == VTSS_ISID_GLOBAL) {
        vtss_appl_psec_global_conf_t global_conf;
        (void)vtss_appl_psec_global_conf_default_get(&global_conf);
        (void)PSEC_LIMIT_global_conf_apply(&global_conf);
    }

    if (default_all || isid != VTSS_ISID_GLOBAL) {
        vtss_isid_t                     isid_start, isid_end;
        mesa_port_no_t                  port;
        vtss_appl_psec_interface_conf_t port_conf;

        // Clear our static entries map. We don't need to remove anything from
        // the PSEC module, because that will be done automatically in just a
        // second.
        psec_limit_mac_map.clear();

        if (default_all) {
            isid_start = VTSS_ISID_START;
            isid_end   = VTSS_ISID_END - 1;
        } else {
            isid_start = isid_end = isid;
        }

        (void)vtss_appl_psec_interface_conf_default_get(&port_conf);

        for (isid = isid_start; isid <= isid_end; isid++) {
            for (port = 0; port < port_count_max(); port++) {
                (void)PSEC_LIMIT_port_conf_apply(isid, port, &port_conf);
            }
        }
    }

    PSEC_LIMIT_MUTEX_ALL_EXIT();
}

/******************************************************************************/
// PSEC_LIMIT_mac_add()
/******************************************************************************/
static mesa_rc PSEC_LIMIT_mac_add(psec_limit_ifindex_t *psec_limit_ifindex, vtss_appl_psec_mac_conf_t *mac_conf)
{
    vtss_appl_psec_interface_conf_t *port_conf = &PSEC_LIMIT_stack_conf.switch_conf[psec_limit_ifindex->isid - VTSS_ISID_START].port_conf[psec_limit_ifindex->port];
    char                            buf[PSEC_LIMIT_MAC_STR_BUF_SIZE];
    mesa_rc                         rc = VTSS_RC_OK;

    if (mac_conf->vlan == VTSS_VID_NULL) {
        T_E("Internal error: VLAN ID not updated");
        return VTSS_RC_ERROR;
    }

    if (port_conf->enabled && PSEC_LIMIT_icfg_loading_post_seen) {
        (void)PSEC_LIMIT_mac_conf_to_str(psec_limit_ifindex->ifindex, mac_conf, buf);

        // The PSEC module will call PSEC_LIMIT_on_mac_add_callback(), and we need
        // to know that this is a special case (and a dirty hack to avoid race
        // conditions and deadlocks).
        T_I("%s: Invoking psec_mgmt_mac_add_special()", buf);
        rc = psec_mgmt_mac_add_special(VTSS_APPL_PSEC_USER_ADMIN, psec_limit_ifindex->ifindex, mac_conf);

        if (rc == VTSS_APPL_PSEC_RC_MAC_POOL_DEPLETED) {
            // Special case, where adding this MAC was not done, because the
            // pool of MACs is currently depleted. This is not a programming
            // error and should therefore not be flagged as a trace error.
            // It will be up to the caller to determine how to handle it.
            // We change it to another error text that is more user-friendly.
            T_I("Add %s: rc = %s", buf, error_txt(rc));
            rc = PSEC_LIMIT_ERROR_MAC_POOL_DEPLETED;
        } else if (rc != VTSS_RC_OK) {
            T_E("Add %s: rc = %s", buf, error_txt(rc));
        }
    }

    return rc;
}

/******************************************************************************/
// PSEC_LIMIT_mac_del()
/******************************************************************************/
static mesa_rc PSEC_LIMIT_mac_del(psec_limit_ifindex_t *psec_limit_ifindex, vtss_appl_psec_mac_conf_t *mac_conf)
{
    vtss_appl_psec_interface_conf_t *port_conf = &PSEC_LIMIT_stack_conf.switch_conf[psec_limit_ifindex->isid - VTSS_ISID_START].port_conf[psec_limit_ifindex->port];
    char                            buf[PSEC_LIMIT_MAC_STR_BUF_SIZE];
    mesa_rc                         rc = VTSS_RC_OK;

    if (mac_conf->vlan == VTSS_VID_NULL) {
        T_E("Internal error: VLAN ID not updated");
        return VTSS_RC_ERROR;
    }

    if (port_conf->enabled && PSEC_LIMIT_icfg_loading_post_seen) {
        // The PSEC module will call PSEC_LIMIT_on_mac_del_callback(), and we need
        // to know that this is a special case (and a dirty hack to avoid race
        // conditions and deadlocks).
        rc = psec_mgmt_mac_del_special(VTSS_APPL_PSEC_USER_ADMIN, psec_limit_ifindex->ifindex, mac_conf);

        if (rc != VTSS_RC_OK) {
            T_E("Delete %s: rc = %s", PSEC_LIMIT_mac_conf_to_str(psec_limit_ifindex->ifindex, mac_conf, buf), error_txt(rc));
        }
    }

    return rc;
}

/******************************************************************************/
// PSEC_LIMIT_svl_conf_change_callback()
/******************************************************************************/
static void PSEC_LIMIT_svl_conf_change_callback(mesa_vid_t vid, mesa_vid_t new_fid)
{
    char                 buf[PSEC_LIMIT_MAC_STR_BUF_SIZE], buf2[PSEC_LIMIT_MAC_STR_BUF_SIZE];
    psec_limit_mac_itr_t mac_itr;
    mesa_vid_t           old_fid;

    PSEC_LIMIT_MUTEX_ALL_ENTER();

    // Get old FID
    old_fid = PSEC_LIMIT_FID_GET(vid);

    // Get new FID again to convert from a possible 0, which means FID == VID.
    if (new_fid == 0) {
        new_fid = vid;
    }

    T_I("VID = %u: FID change from %u to %u", vid, old_fid, new_fid);

    if (old_fid == new_fid) {
        // No FID change
        goto do_exit;
    }

    // Change FID in our cached VID-to-FID table.
    PSEC_LIMIT_FID_SET(vid, new_fid);

    // Look for all entries with vlan = vid and and change their fid to new_fid
    // unless another entry with same MAC maps to the new FID, in which case, we
    // silently delete <old_vid, MAC>.
    mac_itr = PSEC_LIMIT_mac_itr_get_first();
    while (mac_itr != psec_limit_mac_map.end()) {
        psec_limit_mac_itr_t      next_mac_itr = mac_itr, tmp_mac_itr;
        vtss_appl_psec_mac_conf_t mac_conf;
        psec_limit_ifindex_t      psec_limit_ifindex;

        // Save a copy of the next entry, because it might be we remove the
        // current.
        ++next_mac_itr;

        if (mac_itr->first.vlan != vid) {
            // Not affected.
            goto next;
        }

        (void)PSEC_LIMIT_mac_itr_to_str(mac_itr, buf);
        T_I("Considering %s", buf);

        if (PSEC_LIMIT_ifindex_from_ifindex(mac_itr->first.ifindex, &psec_limit_ifindex, __LINE__) != VTSS_RC_OK) {
            goto do_exit;
        }

        // We need to re-install this entry in the PSEC module. Start by
        // deleting it.
        mac_conf = mac_itr->second;

        // First from our own map
        psec_limit_mac_map.erase(mac_itr);

        // Then from the PSEC module
        (void)PSEC_LIMIT_mac_del(&psec_limit_ifindex, &mac_conf);

        // See if there's any such MAC address on the new FID. If so, don't
        // re-add this MAC address, but keep that other entry.
        tmp_mac_itr = PSEC_LIMIT_mac_itr_get_from_fid_mac(new_fid, &mac_itr->first.mac);

        if (tmp_mac_itr != psec_limit_mac_map.end()) {
            T_I("%s: Silently deleting, because another MAC (%s) is found on the new SVL (%u)", buf, PSEC_LIMIT_mac_itr_to_str(tmp_mac_itr, buf2), new_fid);
        } else {
            T_I("%s: Replacing MAC entry from FID = %u to %u", buf, old_fid, new_fid);
            if ((mac_itr = PSEC_LIMIT_mac_itr_alloc(psec_limit_ifindex.ifindex, &mac_conf)) == psec_limit_mac_map.end()) {
                T_I("Out of memory");
            }

            (void)PSEC_LIMIT_mac_add(&psec_limit_ifindex, &mac_conf);
        }

next:
        mac_itr = next_mac_itr;
    }

do_exit:
    // Tell the PSEC module about this. The reason that the PSEC module must get
    // this info from this module is that we can't otherwise be sure that the
    // VLAN module calls back PSEC_LIMIT and PSEC in the right order (PSEC_LIMIT
    // first, so that it can adjust its sticky and static MAC addresses
    // accordingly, even though they are not currently present on a port,
    // because PSEC_LIMIT is not enabled on that port).
    psec_mgmt_fid_change(vid, old_fid, new_fid);

    PSEC_LIMIT_MUTEX_ALL_EXIT();
}

/******************************************************************************/
// PSEC_LIMIT_vlan_port_conf_change_callback()
/******************************************************************************/
static void PSEC_LIMIT_vlan_port_conf_change_callback(vtss_isid_t isid, mesa_port_no_t port_no, const vtss_appl_vlan_port_detailed_conf_t *new_vlan_conf)
{
    psec_limit_ifindex_t psec_limit_ifindex;
    psec_limit_mac_itr_t mac_itr;
    mesa_vid_t           old_pvid, new_pvid = new_vlan_conf->pvid;
    char                 buf[PSEC_LIMIT_MAC_STR_BUF_SIZE];

    PSEC_LIMIT_MUTEX_ALL_ENTER();

    old_pvid = PSEC_LIMIT_pvids[port_no];

    T_I("Port = %u: old PVID = %u, new PVID = %u", port_no, old_pvid, new_pvid);

    if (new_pvid == old_pvid) {
        // No PVID change
        goto do_exit;
    }

    if (PSEC_LIMIT_ifindex_from_port(isid, port_no, &psec_limit_ifindex, __LINE__) != VTSS_RC_OK) {
        goto do_exit;
    }

    // Look for all entries with vlan = old_pvid and change them to new_pvid
    // unless another entry with <new_pvid, MAC> already exists, in which case,
    // we silently delete <old_pvid, MAC>.
    mac_itr = PSEC_LIMIT_mac_itr_get_first_from_ifindex(psec_limit_ifindex.ifindex);
    while (mac_itr != psec_limit_mac_map.end()) {
        psec_limit_mac_itr_t next_mac_itr = mac_itr;

        if (mac_itr->first.ifindex > psec_limit_ifindex.ifindex) {
            // Done
            break;
        }

        // Save a copy of the next entry, because it might be we remove the
        // current.
        ++next_mac_itr;

        T_I("Considering %s", PSEC_LIMIT_mac_itr_to_str(mac_itr, buf));

        if (mac_itr->first.vlan == old_pvid) {
            // Before changing, check that there's no such entry on new_pvid's
            // FID. If there is, delete the current entry.
            vtss_appl_psec_mac_conf_t old_mac_conf, new_mac_conf;
            psec_limit_mac_itr_t      tmp_mac_itr;
            mesa_vid_t                new_fid = PSEC_LIMIT_FID_GET(new_pvid);

            old_mac_conf      = mac_itr->second;
            new_mac_conf      = mac_itr->second;
            new_mac_conf.vlan = new_pvid;

            // Erase the old entry from our map
            psec_limit_mac_map.erase(mac_itr);

            // And from the PSEC module
            (void)PSEC_LIMIT_mac_del(&psec_limit_ifindex, &old_mac_conf);

            // See if the new already exists. If so, don't create a new entry
            // entry, but keep the existing - even if it's installed on
            // another interface.
            tmp_mac_itr = PSEC_LIMIT_mac_itr_get_from_fid_mac(new_fid, &mac_itr->first.mac);
            if (tmp_mac_itr != psec_limit_mac_map.end()) {
                // This doesn't require a re-entrance of the loop, since we've
                // just erased the current iterator and we have the next in
                // next_mac_itr.
                T_I("MAC already exists (%s) on FID %u. Deleting that on VLAN = %u", PSEC_LIMIT_mac_itr_to_str(tmp_mac_itr, buf), new_fid, old_pvid);
            } else {
                // Create a new entry with the new key
                T_I("Deleting on old VLAN and creating on new");
                if ((mac_itr = PSEC_LIMIT_mac_itr_alloc(psec_limit_ifindex.ifindex, &new_mac_conf)) == psec_limit_mac_map.end()) {
                    T_I("Out of memory");
                }

                (void)PSEC_LIMIT_mac_add(&psec_limit_ifindex, &new_mac_conf);
            }
        } else {
            T_I("Not affected");
        }

        mac_itr = next_mac_itr;
    }

    // Update with the latest Port VLAN ID
    PSEC_LIMIT_pvids[port_no] = new_pvid;

do_exit:
    PSEC_LIMIT_MUTEX_ALL_EXIT();
}

/******************************************************************************/
// PSEC_LIMIT_pvid_get()
/******************************************************************************/
mesa_vid_t PSEC_LIMIT_pvid_get(mesa_port_no_t port)
{
    vtss_appl_vlan_port_conf_t vlan_port_conf;
    mesa_vid_t                 pvid;
    mesa_rc                    rc;

    // See if we have already cached the VLAN ID.
    if (PSEC_LIMIT_pvids[port] != VTSS_VID_NULL) {
        // We have. Go use it.
        return PSEC_LIMIT_pvids[port];
    }

    // We haven't. Go cache it. We use the resulting PVID (not necessarily the
    // PVID configured by the admin.
    if ((rc = vlan_mgmt_port_conf_get(VTSS_ISID_START, port, &vlan_port_conf, VTSS_APPL_VLAN_USER_ALL, TRUE)) != VTSS_RC_OK) {
        T_E("Unable to get VLAN port conf for port %u. Defaulting to default VLAN ID (%u)", port, VTSS_APPL_VLAN_ID_DEFAULT);
        pvid = VTSS_APPL_VLAN_ID_DEFAULT;
    } else {
        pvid = vlan_port_conf.hybrid.pvid;
    }

    PSEC_LIMIT_pvids[port] = pvid;
    return pvid;
}

/******************************************************************************/
//
// Module semi-public Functions
//
/******************************************************************************/

/******************************************************************************/
// psec_limit_mgmt_static_macs_get()
//
// Invoked by PSEC module whenever a port comes back up or someone enables
// themselves (including ourselves) as PSEC user, in which case all existing
// MACs get deleted.
//
// The function only gets invoked if PSEC_LIMIT is enabled on the interface.
/******************************************************************************/
void psec_limit_mgmt_static_macs_get(vtss_ifindex_t ifindex, BOOL force)
{
    psec_limit_mac_itr_t mac_itr;
    psec_limit_ifindex_t psec_limit_ifindex;
    char                 buf[PSEC_LIMIT_MAC_STR_BUF_SIZE];

    T_I("Enter, ifindex = %u", VTSS_IFINDEX_PRINTF_ARG(ifindex));

    if (PSEC_LIMIT_ifindex_from_ifindex(ifindex, &psec_limit_ifindex, __LINE__) != VTSS_RC_OK) {
        return;
    }

    if (force) {
        // We have asked PSEC module to call us back, so our mutex is already
        // taken
        PSEC_LIMIT_CRIT_ASSERT_LOCKED();
    } else {
        // PSEC module calls us back as a result of link up. Take mutex.
        PSEC_LIMIT_CRIT_ENTER();
    }

    for (mac_itr = PSEC_LIMIT_mac_itr_get_first_from_ifindex(ifindex); mac_itr != psec_limit_mac_map.end(); ++mac_itr) {
        if (mac_itr->first.ifindex > ifindex) {
            break;
        }

        T_I("Adding %s", PSEC_LIMIT_mac_itr_to_str(mac_itr, buf));
        (void)PSEC_LIMIT_mac_add(&psec_limit_ifindex, &mac_itr->second);
    }

    if (!force) {
        PSEC_LIMIT_CRIT_EXIT();
    }
}

/******************************************************************************/
// psec_limit_mgmt_debug_ref_cnt_get()
/******************************************************************************/
mesa_rc psec_limit_mgmt_debug_ref_cnt_get(vtss_ifindex_t ifindex, u32 *fwd_cnt, u32 *blk_cnt)
{
    psec_limit_ifindex_t psec_limit_ifindex;

    if (!fwd_cnt || !blk_cnt) {
        return PSEC_LIMIT_ERROR_INV_PARAM;
    }

    VTSS_RC(PSEC_LIMIT_ifindex_from_ifindex(ifindex, &psec_limit_ifindex, __LINE__));

    PSEC_LIMIT_CRIT_ENTER();
    *fwd_cnt = PSEC_LIMIT_ref_cnt[psec_limit_ifindex.isid - VTSS_ISID_START][psec_limit_ifindex.port][0];
    *blk_cnt = PSEC_LIMIT_ref_cnt[psec_limit_ifindex.isid - VTSS_ISID_START][psec_limit_ifindex.port][1];
    PSEC_LIMIT_CRIT_EXIT();

    return VTSS_RC_OK;
}

/******************************************************************************/
// psec_limit_mgmt_fid_get()
/******************************************************************************/
mesa_vid_t psec_limit_mgmt_fid_get(mesa_vid_t vid)
{
    // The PSEC_LIMIT_vid_to_fid[] array is protected by the PSEC mutex, which
    // must be taken by now by the PSEC module.
    return PSEC_LIMIT_FID_GET(vid);
}

/******************************************************************************/
//
// Module Public Functions
//
/******************************************************************************/

/******************************************************************************/
// vtss_appl_psec_global_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_psec_global_conf_default_get(vtss_appl_psec_global_conf_t *const global_conf)
{
    if (!global_conf) {
        return PSEC_LIMIT_ERROR_INV_PARAM;
    }

    // By default, aging is disabled.
    memset(global_conf, 0, sizeof(*global_conf));
    global_conf->aging_period_secs = 3600; // 1 hour
    global_conf->hold_time_secs    =  300; // 5 minutes

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psec_global_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_psec_global_conf_get(vtss_appl_psec_global_conf_t *const global_conf)
{
    mesa_rc rc;

    if (!global_conf) {
        return PSEC_LIMIT_ERROR_INV_PARAM;
    }

    if ((rc = PSEC_LIMIT_primary_switch_isid_port_check(VTSS_ISID_START, 0, FALSE)) != VTSS_RC_OK) {
        return rc;
    }

    PSEC_LIMIT_CRIT_ENTER();
    *global_conf = PSEC_LIMIT_stack_conf.global_conf;
    PSEC_LIMIT_CRIT_EXIT();

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psec_global_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_psec_global_conf_set(const vtss_appl_psec_global_conf_t *global_conf)
{
    mesa_rc rc;

    if (!global_conf) {
        return PSEC_LIMIT_ERROR_INV_PARAM;
    }

    if ((rc = PSEC_LIMIT_primary_switch_isid_port_check(VTSS_ISID_START, 0, FALSE)) != VTSS_RC_OK) {
        return rc;
    }

    if (global_conf->aging_period_secs < PSEC_AGE_TIME_MIN || global_conf->aging_period_secs > PSEC_AGE_TIME_MAX) {
        return PSEC_LIMIT_ERROR_INV_AGING_PERIOD;
    }

    if (global_conf->hold_time_secs < PSEC_HOLD_TIME_MIN || global_conf->hold_time_secs > PSEC_HOLD_TIME_MAX) {
        return PSEC_LIMIT_ERROR_INV_HOLD_TIME;
    }

    // Gotta get the PSEC module's mutex first so that it doesn't add new MAC
    // addresses behind our back - and in order to avoid deadlocks.
    PSEC_LIMIT_MUTEX_ALL_ENTER();
    rc = PSEC_LIMIT_global_conf_apply(global_conf);
    PSEC_LIMIT_MUTEX_ALL_EXIT();

    return rc;
}

/******************************************************************************/
// vtss_appl_psec_interface_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_psec_interface_conf_default_get(vtss_appl_psec_interface_conf_t *const port_conf)
{
    if (!port_conf) {
        return PSEC_LIMIT_ERROR_INV_PARAM;
    }

    memset(port_conf, 0, sizeof(*port_conf));
    port_conf->limit = 4;
    port_conf->violate_limit = 4;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psec_interface_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_psec_interface_conf_get(vtss_ifindex_t ifindex, vtss_appl_psec_interface_conf_t *const port_conf)
{
    vtss_ifindex_elm_t ife;
    mesa_rc            rc;

    if (!port_conf) {
        return PSEC_LIMIT_ERROR_INV_PARAM;
    }

    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK) {
        return PSEC_LIMIT_ERROR_INV_PARAM;
    }

    if (ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
        return PSEC_LIMIT_ERROR_INV_PARAM;
    }

    if ((rc = PSEC_LIMIT_primary_switch_isid_port_check(ife.isid, ife.ordinal, TRUE)) != VTSS_RC_OK) {
        return rc;
    }

    PSEC_LIMIT_CRIT_ENTER();
    *port_conf = PSEC_LIMIT_stack_conf.switch_conf[ife.isid - VTSS_ISID_START].port_conf[ife.ordinal];
    PSEC_LIMIT_CRIT_EXIT();

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psec_interface_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_psec_interface_conf_set(vtss_ifindex_t ifindex, const vtss_appl_psec_interface_conf_t *port_conf)
{
    psec_limit_ifindex_t psec_limit_ifindex;
    mesa_rc              rc = VTSS_RC_OK;

    if (!port_conf) {
        return PSEC_LIMIT_ERROR_INV_PARAM;
    }

    VTSS_RC(PSEC_LIMIT_ifindex_from_ifindex(ifindex, &psec_limit_ifindex, __LINE__, FALSE));

    if ((rc = PSEC_LIMIT_primary_switch_isid_port_check(psec_limit_ifindex.isid, psec_limit_ifindex.port, TRUE)) != VTSS_RC_OK) {
        return rc;
    }

    if ((rc = PSEC_LIMIT_conf_valid_port(port_conf)) != VTSS_RC_OK) {
        return rc;
    }

    PSEC_LIMIT_MUTEX_ALL_ENTER();

    // Additional check: We can't change the limit to a value smaller than
    // current number of learned MAC addresses.
    // We must use the upcoming enabled and limit values, and not the current.
    if ((rc = PSEC_LIMIT_check_limit(&psec_limit_ifindex, port_conf->enabled, port_conf->limit, FALSE)) != VTSS_RC_OK) {
        goto do_exit;
    }

    rc = PSEC_LIMIT_port_conf_apply(psec_limit_ifindex.isid, psec_limit_ifindex.port, port_conf);

do_exit:
    PSEC_LIMIT_MUTEX_ALL_EXIT();

    return rc;
}

/******************************************************************************/
// vtss_appl_psec_interface_conf_mac_default_get()
/******************************************************************************/
mesa_rc vtss_appl_psec_interface_conf_mac_default_get(vtss_ifindex_t *ifindex, mesa_vid_t *vlan, mesa_mac_t *mac, vtss_appl_psec_mac_conf_t *const mac_conf)
{
    if (!ifindex || !vlan || !mac || !mac_conf) {
        return PSEC_LIMIT_ERROR_INV_PARAM;
    }

    memset(mac,      0, sizeof(*mac));
    memset(mac_conf, 0, sizeof(*mac_conf));

    // Can't use vtss_ifindex_from_port() here, because that function is invoked
    // by the SNMP framework prior to knowing the number of ports on the device.
    // So gotta circumvent a call to that function.
    *ifindex = vtss_ifindex_cast_from_u32(VTSS_IFINDEX_START_           +
                                          VTSS_IFINDEX_UNIT_MULTP_      +
                                          VTSS_IFINDEX_PORT_UNIT_OFFSET_,
                                          VTSS_IFINDEX_TYPE_PORT);

    *vlan    = VTSS_APPL_VLAN_ID_DEFAULT;
    mac->addr[5] = 0x01;
    mac_conf->mac_type = VTSS_APPL_PSEC_MAC_TYPE_STATIC;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psec_interface_conf_mac_add()
/******************************************************************************/
mesa_rc vtss_appl_psec_interface_conf_mac_add(vtss_ifindex_t ifindex, mesa_vid_t vlan, mesa_mac_t mac, const vtss_appl_psec_mac_conf_t *const mac_conf)
{
    vtss_appl_psec_interface_conf_t *port_conf;
    vtss_appl_psec_mac_conf_t       auto_mac_conf;
    psec_limit_ifindex_t            psec_limit_ifindex;
    psec_limit_mac_itr_t            mac_itr;
    BOOL                            entry_already_exists;
    mesa_vid_t                      fid;
    mesa_rc                         rc = VTSS_RC_OK;
    char                            buf[PSEC_LIMIT_MAC_STR_BUF_SIZE];

    if (!mac_conf) {
        return PSEC_LIMIT_ERROR_INV_PARAM;
    }

    VTSS_RC(PSEC_LIMIT_ifindex_from_ifindex(ifindex, &psec_limit_ifindex, __LINE__, FALSE));

    // It must be a unicast MAC address
    if (mac.addr[0] & 0x1) {
        return PSEC_LIMIT_ERROR_MAC_NOT_UNICAST;
    }

    // We gotta be able to change it before adding it in case VLAN is
    // VTSS_VID_NULL in which case the user wants to install it on the PVID.
    // Also, the configuration itself needs to contain the MAC and VLAN even
    // though these are also used as args to this function.
    auto_mac_conf = *mac_conf;
    auto_mac_conf.vlan = vlan;
    auto_mac_conf.mac  = mac;

    T_I("Adding %s as %s", PSEC_LIMIT_mac_conf_to_str(ifindex, &auto_mac_conf, buf), psec_util_mac_type_to_str(mac_conf->mac_type));

    PSEC_LIMIT_MUTEX_ALL_ENTER();

    if (auto_mac_conf.vlan == VTSS_VID_NULL) {
        // This is our way to signal that this MAC address follows PVID.
        auto_mac_conf.vlan = PSEC_LIMIT_pvid_get(psec_limit_ifindex.port);
        T_I("%s: Changed VLAN", PSEC_LIMIT_mac_conf_to_str(ifindex, &auto_mac_conf, buf));
    }

    port_conf = &PSEC_LIMIT_stack_conf.switch_conf[psec_limit_ifindex.isid - VTSS_ISID_START].port_conf[psec_limit_ifindex.port];

    // Search our local table for this <FID, MAC>. Later on we will ask PSEC
    // whether the entry is already found.
    fid = PSEC_LIMIT_FID_GET(vlan);
    mac_itr = PSEC_LIMIT_mac_itr_get_from_fid_mac(fid, &mac);
    entry_already_exists = mac_itr != psec_limit_mac_map.end();

    if (entry_already_exists && mac_itr->first.vlan != vlan) {
        // Entry already exists, but on another VLAN that maps to the same FID.
        rc = VTSS_APPL_PSEC_RC_MAC_VID_ALREADY_FOUND_ON_SVL;
        goto do_exit;
    }

    if (entry_already_exists && mac_itr->first.ifindex != psec_limit_ifindex.ifindex) {
        // Entry already found on another interface.
        rc = VTSS_APPL_PSEC_RC_MAC_VID_ALREADY_FOUND;
        goto do_exit;
    }

    if (auto_mac_conf.mac_type == VTSS_APPL_PSEC_MAC_TYPE_STICKY && !port_conf->sticky) {
        // Can't add a sticky MAC to a non-sticky interface
        rc = PSEC_LIMIT_ERROR_STICKY_ENTRY_ON_NON_STICKY_PORT;
        goto do_exit;
    }

    if (auto_mac_conf.mac_type == VTSS_APPL_PSEC_MAC_TYPE_DYNAMIC) {
        if (!port_conf->enabled) {
            // Can't add dynamic entry on a PSEC-disabled port.
            rc = PSEC_LIMIT_ERROR_DYNAMIC_ENTRY_ON_PSEC_DISABLED_PORT;
            goto do_exit;
        }

        if (port_conf->sticky) {
            // Can't add dynamic entry to a sticky interface
            rc = PSEC_LIMIT_ERROR_DYNAMIC_ENTRY_ON_STICKY_PORT;
            goto do_exit;
        }

        if (entry_already_exists) {
            // Our own map only contains static and sticky entries.
            // Cannot use this function to change such an entry to dynamic, that
            // is, user has to delete the static/sticky first before calling
            // this function.
            // Here, the entry can only exist if it is static, not sticky,
            // because we've just checked that the port is not sticky, so the
            // map cannot contain sticky entries.
            rc = PSEC_LIMIT_ERROR_DYNAMIC_REPLACE_STATIC;
            goto do_exit;
        }
    }

    if (entry_already_exists) {
        // Here, auto_mac_conf.mac_type is either static or sticky, and so is
        // mac_itr->second.mac_type.
        // Playing with the feature on other switches has revealed that it's
        // not possible to change a sticky MAC address to a static and vice
        // versa, but I think it's a good feature to be able to do that (at
        // least sticky to static).
        mac_itr->second.mac_type = auto_mac_conf.mac_type;
        goto do_exit;
    }

    // We can't add another entry if limit is reached.
    if ((rc = PSEC_LIMIT_check_limit(&psec_limit_ifindex, port_conf->enabled, port_conf->limit, TRUE)) != VTSS_RC_OK) {
        goto do_exit;
    }

    if (auto_mac_conf.mac_type != VTSS_APPL_PSEC_MAC_TYPE_DYNAMIC) {
        // Add the entry to our own map.
        if (PSEC_LIMIT_mac_itr_alloc(ifindex, &auto_mac_conf) == psec_limit_mac_map.end()) {
            rc = VTSS_APPL_PSEC_RC_OUT_OF_MEMORY;
            goto do_exit;
        }
    }

    rc = PSEC_LIMIT_mac_add(&psec_limit_ifindex, &auto_mac_conf);

do_exit:
    PSEC_LIMIT_MUTEX_ALL_EXIT();

    return rc;
}

/******************************************************************************/
// vtss_appl_psec_interface_conf_mac_del()
/******************************************************************************/
mesa_rc vtss_appl_psec_interface_conf_mac_del(vtss_ifindex_t ifindex, mesa_vid_t vlan, mesa_mac_t mac)
{
    vtss_appl_psec_mac_conf_t mac_conf;
    psec_limit_ifindex_t      psec_limit_ifindex;
    psec_limit_mac_itr_t      mac_itr;
    mesa_rc                   rc;

    VTSS_RC(PSEC_LIMIT_ifindex_from_ifindex(ifindex, &psec_limit_ifindex, __LINE__, FALSE));

    // We gotta be able to change it before looking it up in case VLAN is
    // VTSS_VID_NULL in which case the user wants to remove it from PVID.
    memset(&mac_conf, 0, sizeof(mac_conf));
    mac_conf.vlan    = vlan;
    mac_conf.mac     = mac;

    PSEC_LIMIT_MUTEX_ALL_ENTER();

    if (mac_conf.vlan == VTSS_VID_NULL) {
        // This is our way to signal that this MAC address follows PVID.
        mac_conf.vlan = PSEC_LIMIT_pvid_get(psec_limit_ifindex.port);
    }

    if ((mac_itr = PSEC_LIMIT_mac_itr_get_from_mac_conf(&mac_conf)) == psec_limit_mac_map.end()) {
        rc = PSEC_LIMIT_ERROR_NO_SUCH_ENTRY;
        goto do_exit;
    }

    if (mac_itr->first.ifindex != ifindex) {
        rc = PSEC_LIMIT_ERROR_ENTRY_FOUND_ON_ANOTHER_INTERFACE;
        goto do_exit;
    }

    psec_limit_mac_map.erase(mac_itr);

    rc = PSEC_LIMIT_mac_del(&psec_limit_ifindex, &mac_conf);

do_exit:
    PSEC_LIMIT_MUTEX_ALL_EXIT();
    return rc;
}

/******************************************************************************/
// vtss_appl_psec_interface_conf_mac_pvid_get()
/******************************************************************************/
mesa_rc vtss_appl_psec_interface_conf_mac_pvid_get(vtss_ifindex_t ifindex, mesa_vid_t vid, mesa_mac_t mac, vtss_appl_psec_mac_conf_t *mac_conf, mesa_vid_t *pvid)
{
    psec_limit_ifindex_t     psec_limit_ifindex;
    psec_limit_mac_itr_t     mac_itr;
    psec_limit_mac_map_key_t key;
    mesa_rc                  rc = VTSS_RC_OK;

    if (!mac_conf || !pvid) {
        return VTSS_APPL_PSEC_RC_INV_PARAM;
    }

    VTSS_RC(PSEC_LIMIT_ifindex_from_ifindex(ifindex, &psec_limit_ifindex, __LINE__, FALSE));
    *pvid = PSEC_LIMIT_pvid_get(psec_limit_ifindex.port);

    PSEC_LIMIT_CRIT_ENTER();

    if (vid == VTSS_VID_NULL) {
        vid = *pvid;
    }

    memset(&key, 0, sizeof(key));
    key.ifindex = ifindex;
    key.vlan    = vid;
    key.mac     = mac;

    if ((mac_itr = PSEC_LIMIT_mac_itr_get(&key)) == psec_limit_mac_map.end()) {
        // Not found
        rc = VTSS_APPL_PSEC_RC_MAC_VID_NOT_FOUND;
        goto do_exit;
    }

    *mac_conf = mac_itr->second;

do_exit:
    PSEC_LIMIT_CRIT_EXIT();
    return rc;
}

/******************************************************************************/
// vtss_appl_psec_interface_conf_mac_get()
/******************************************************************************/
mesa_rc vtss_appl_psec_interface_conf_mac_get(vtss_ifindex_t ifindex, mesa_vid_t vid, mesa_mac_t mac, vtss_appl_psec_mac_conf_t *mac_conf)
{
    mesa_vid_t pvid; // Not used
    return vtss_appl_psec_interface_conf_mac_pvid_get(ifindex, vid, mac, mac_conf, &pvid);
}

/******************************************************************************/
// vtss_appl_psec_interface_conf_mac_itr()
// Could have used vtss::iteratorComposeN, but that would cause billions and
// billions of calls into vtss_appl_psec_interface_conf_mac_get(), because
// all ifindices, all VIDs, and all possible MAC addresses would have to be
// tried out - most of them returning false.
/******************************************************************************/
mesa_rc vtss_appl_psec_interface_conf_mac_itr(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex,
                                              const mesa_vid_t     *prev_vid,     mesa_vid_t     *next_vid,
                                              const mesa_mac_t     *prev_mac,     mesa_mac_t     *next_mac)
{
    char                 mac_str[18];
    psec_limit_mac_itr_t mac_itr;
    mesa_rc              rc;

    T_N("prev_ifindex = %u, prev_vid = %u, prev_mac = %s", prev_ifindex ? VTSS_IFINDEX_PRINTF_ARG(*prev_ifindex) : -1, prev_vid ? *prev_vid : -1, prev_mac ? misc_mac_txt(prev_mac->addr, mac_str) : "(NULL)");

    if (!next_ifindex || !next_vid || !next_mac) {
        T_E("Invalid next pointer");
        return VTSS_RC_ERROR;
    }

    PSEC_LIMIT_CRIT_ENTER();

    // If prev_ifindex is NULL, then it's guaranteed that so are prev_vid and prev_mac
    if (!prev_ifindex) {
        mac_itr = PSEC_LIMIT_mac_itr_get_first();
        goto do_exit;
    }

    // Here, we have a valid prev_ifindex. Start with that one
    if ((mac_itr = PSEC_LIMIT_mac_itr_get_first_from_ifindex(*prev_ifindex)) == psec_limit_mac_map.end()) {
        mac_itr = PSEC_LIMIT_mac_itr_get_next_from_ifindex(*prev_ifindex);
    }

    if (mac_itr == psec_limit_mac_map.end()) {
        goto do_exit;
    }

    if (mac_itr->first.ifindex != *prev_ifindex || !prev_vid) {
        // This one is on the next ifindex or the caller hasn't got a VID preference.
        // Anyhow, exit with success.
        goto do_exit;
    }

    // Here, the user has a VID preference.
    // Search for the first MAC on this ifindex with a VID == *prev_vid
    while (mac_itr != psec_limit_mac_map.end() && mac_itr->first.ifindex == *prev_ifindex && mac_itr->first.vlan < *prev_vid) {
        mac_itr = ++mac_itr;
    }

    if (mac_itr == psec_limit_mac_map.end()) {
        goto do_exit;
    }

    if (mac_itr->first.ifindex != *prev_ifindex || mac_itr->first.vlan != *prev_vid || !prev_mac) {
        // This one is on the next ifindex or the next vid or the caller hasn't got a MAC preference.
        goto do_exit;
    }

    // Here, the user has a MAC preference.
    // Search for the first MAC with a MAC address > the *prev_mac
    while (mac_itr != psec_limit_mac_map.end() && mac_itr->first.ifindex == *prev_ifindex && mac_itr->first.vlan == *prev_vid && memcmp(&mac_itr->first.mac, prev_mac, sizeof(mac_itr->first.mac)) <= 0) {
        mac_itr = ++mac_itr;
    }

do_exit:
    if (mac_itr != psec_limit_mac_map.end()) {
        *next_ifindex = mac_itr->first.ifindex;
        *next_vid     = mac_itr->first.vlan;
        *next_mac     = mac_itr->first.mac;
        T_N("Exit. Found one: next_ifindex = %u, next_vid = %u, next_mac = %s", VTSS_IFINDEX_PRINTF_ARG(*next_ifindex), *next_vid, misc_mac_txt(next_mac->addr, mac_str));
        rc = VTSS_RC_OK;
    } else {
        T_N("Exit. No next");
        rc = VTSS_RC_ERROR;
    }

    PSEC_LIMIT_CRIT_EXIT();

    return rc;
}

/******************************************************************************/
// psec_limit_error_txt()
/******************************************************************************/
const char *psec_limit_error_txt(mesa_rc rc)
{
    switch (rc) {
    case PSEC_LIMIT_ERROR_INV_PARAM:
        return "Invalid parameter";

    case PSEC_LIMIT_ERROR_MUST_BE_PRIMARY_SWITCH:
        return "Operation only valid on the primary switch";

    case PSEC_LIMIT_ERROR_INV_ISID:
        return "Invalid Switch ID";

    case PSEC_LIMIT_ERROR_INV_PORT:
        return "Invalid port number";

    case PSEC_LIMIT_ERROR_INV_IFINDEX:
        return "Unable to decompose ifindex";

    case PSEC_LIMIT_ERROR_IFINDEX_NOT_REPRESENTING_A_PORT:
        return "Ifindex not representing a port";

    case PSEC_LIMIT_ERROR_INV_AGING_PERIOD:
        return "Aging period out of bounds";

    case PSEC_LIMIT_ERROR_INV_HOLD_TIME:
        return "Hold time out of bounds";

    case PSEC_LIMIT_ERROR_INV_LIMIT:
        return "Invalid limit";

    case PSEC_LIMIT_ERROR_INV_VIOLATE_LIMIT:
        return "Invalid violation limit";

    case PSEC_LIMIT_ERROR_INV_VIOLATION_MODE:
        return "The violation mode is out of bounds";

    case PSEC_LIMIT_ERROR_STATIC_AGGR_ENABLED:
        return "Limit control cannot be enabled for ports that are enabled for static aggregation";

    case PSEC_LIMIT_ERROR_DYNAMIC_AGGR_ENABLED:
        return "Limit control cannot be enabled for ports that are enabled for LACP";

    case PSEC_LIMIT_ERROR_DYNAMIC_ENTRY_ON_PSEC_DISABLED_PORT:
        return "Cannot add dynamic entry to port-security-disabled port";

    case PSEC_LIMIT_ERROR_DYNAMIC_ENTRY_ON_STICKY_PORT:
        return "Cannot add dynamic entry to sticky interface";

    case PSEC_LIMIT_ERROR_STICKY_ENTRY_ON_NON_STICKY_PORT:
        return "Cannot add sticky entry on non-sticky interface";

    case PSEC_LIMIT_ERROR_DYNAMIC_REPLACE_STATIC:
        return "Dynamic entry cannot replace a static entry";

    case PSEC_LIMIT_ERROR_CANT_DELETE_DYNAMIC_ENTRIES:
        return "Unable to remove dynamic entries this way";

    case PSEC_LIMIT_ERROR_NO_SUCH_ENTRY:
        return "No such entry";

    case PSEC_LIMIT_ERROR_ENTRY_FOUND_ON_ANOTHER_INTERFACE:
        return "Entry found on another interface";

    case PSEC_LIMIT_ERROR_LIMIT_LOWER_THAN_CUR_CNT:
        return "Maximum cannot be set to a value lower that the current number of secured MAC addresses";

    case PSEC_LIMIT_ERROR_STATIC_STICKY_MAC_CNT_EXCEEDS_GLOBAL_POOL_SIZE:
        return "Adding this MAC address will cause the total number of static or sticky MACs to exceed the size of the global pool";

    case PSEC_LIMIT_ERROR_MAC_POOL_DEPLETED:
        return "Adding this MAC address to the configuration, but cannot currently add it to hardware, because the global pool of MAC addresses is depleted";

    case PSEC_LIMIT_ERROR_MAC_NOT_UNICAST:
        return "MAC address must be a unicast MAC address";

    default:
        return "Port Security Limit Control: Unknown error code";
    }
}

extern "C" int psec_limit_icli_cmd_register();

/******************************************************************************/
// psec_limit_init()
/******************************************************************************/
mesa_rc psec_limit_init(vtss_init_data_t *data)
{
    vtss_isid_t    isid = data->isid;
    mesa_port_no_t port;
    mesa_rc        rc;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        // Initialize sempahores.
        critd_init(&PSEC_LIMIT_crit, "psec_limit", VTSS_MODULE_ID_PSEC_LIMIT, CRITD_TYPE_MUTEX);

        psec_limit_icli_cmd_register();
        break;

    case INIT_CMD_START:
        // We must also install callback handlers in the psec module. We will then be called
        // whenever the psec module adds MAC addresses to the MAC table. We don't care
        // when the psec module deletes a MAC address from the MAC table (if the port limit is
        // currently reached, the PSEC module will autonomously clear the reached limit flag
        // when deleting a MAC address).
        // Do this as soon as possible in the boot process.
        if ((rc = psec_mgmt_register_callbacks(VTSS_APPL_PSEC_USER_ADMIN, PSEC_LIMIT_on_mac_add_callback, PSEC_LIMIT_on_mac_del_callback)) != VTSS_RC_OK) {
            T_E("Unable to register callbacks (%s)", error_txt(rc));
        }

        // Register for Shared VLAN Learning changes.
        vlan_svl_conf_change_register(VTSS_MODULE_ID_PSEC_LIMIT, PSEC_LIMIT_svl_conf_change_callback);

        // Register for Port VLAN ID changes. When we register in
        // INIT_CMD_START, the VLAN module will automatically call us back in
        // INIT_CMD_ICFG_LOADING_PRE with default settings, so no need to
        // populate the PVID table manually.
        vlan_port_conf_change_register(VTSS_MODULE_ID_PSEC_LIMIT, PSEC_LIMIT_vlan_port_conf_change_callback, TRUE);

#ifdef VTSS_SW_OPTION_ICFG
        VTSS_RC(psec_limit_icfg_init()); // ICFG initialization (Show running)
#endif
        break;

    case INIT_CMD_CONF_DEF:
        if (isid == VTSS_ISID_LOCAL) {
            // Reset local configuration
            // No such configuration for this module
        } else if (VTSS_ISID_LEGAL(isid) || isid == VTSS_ISID_GLOBAL) {
            // Reset switch or stack configuration
            PSEC_LIMIT_default(isid, FALSE);
        }
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        PSEC_LIMIT_default(VTSS_ISID_GLOBAL, TRUE);
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        PSEC_LIMIT_icfg_loading_post_seen = TRUE;
        PSEC_LIMIT_MUTEX_ALL_ENTER();

        // Force-apply whatever we've been configured with so far.
        (void)PSEC_LIMIT_global_conf_apply(NULL);
        for (port = 0; port < port_count_max(); port++) {
            (void)PSEC_LIMIT_port_conf_apply(isid, port, NULL);
        }

        PSEC_LIMIT_MUTEX_ALL_EXIT();
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

