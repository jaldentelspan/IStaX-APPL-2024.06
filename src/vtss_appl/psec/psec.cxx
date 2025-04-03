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

// See PSEC_msg_rx() for an in-depth discussion of why this is OK.
/*lint -esym(459, PSEC_msg_rx) */

#include <time.h>            /* For time_t                                           */
#include "critd_api.h"       /* For mutex wrapper                                    */
#include "psec_api.h"        /* To get access to our own structures and enumerations */
#include <psec_util.h>       /* For psec_util_mac_type_to_str()                      */
#include "psec_limit_api.h"  /* For psec_limit_mgmt_static_macs_get()                */
#include "msg_api.h"         /* For message transmission and reception functions.    */
#include "psec_rate_limit.h" /* For rate-limiter                                     */
#include "misc_api.h"        /* For misc_mac_txt()                                   */
#include "mac_api.h"         /* mac_mgmt_learn_mode_XXX() MAC functions              */
#include "packet_api.h"      /* For packet_rx_filter_XXX()                           */
#include "port_api.h"        /* For port_vol_conf_set()                              */
#include "port_iter.hxx"     /* For port iterators                                   */
#include "main.h"            /* For vtss_xstr()                                      */



#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"      /* For S_W()                                            */
#endif
#include "psec_trace.h"
#if defined(VTSS_SW_OPTION_MSTP)
#include "l2proto_api.h"
#include "mstp_api.h"
#endif /* VTSS_SW_OPTION_MSTP */

// In order to have the Linux Kernel assess whether to forward IP frames from
// particular ports or MAC addresses to the IP stack, we need to insert filters
// into the kernel's frame Mux.
#include "ip_filter_api.hxx"

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_PSEC

/****************************************************************************/
// Trace definitions
/****************************************************************************/
#include <vtss_trace_api.h>
#include "psec_expose.hxx"
#include <vtss/basics/memcmp-operator.hxx>     /* For VTSS_BASICS_MEMCMP_OPERATOR        */
#include <vtss/basics/expose/table-status.hxx> /* For vtss::expose::TableStatus          */
#include "subject.hxx"                         /* For notifications::subject_main_thread */

using namespace vtss;

/* Trace registration. Initialized by psec_init() */
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "psec", "Port Security module"
};

#ifndef PSEC_DEFAULT_TRACE_LVL
#define PSEC_DEFAULT_TRACE_LVL VTSS_TRACE_LVL_ERROR
#endif

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        PSEC_DEFAULT_TRACE_LVL
    },
    [TRACE_GRP_MAC_MODULE] = {
        "mac",
        "MAC Module Calls",
        PSEC_DEFAULT_TRACE_LVL
    },
    [TRACE_GRP_IP_FILTER] = {
        "ip_filter",
        "IP Filter Calls",
        PSEC_DEFAULT_TRACE_LVL
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/******************************************************************************/
// Mutex stuff.
/******************************************************************************/
static critd_t crit_psec;

// Macros for accessing mutex functions
// ------------------------------------
#define PSEC_CRIT_ENTER()         critd_enter(        &crit_psec, __FILE__, __LINE__)
#define PSEC_CRIT_EXIT()          critd_exit(         &crit_psec, __FILE__, __LINE__)
#define PSEC_CRIT_ASSERT_LOCKED() critd_assert_locked(&crit_psec, __FILE__, __LINE__)

// Alternative way of locking: Lock a scope and automatically unlock when leaving
struct PSEC_Lock {
    PSEC_Lock(int line)
    {
        critd_enter(&crit_psec, __FILE__, line);
    }

    ~PSEC_Lock()
    {
        critd_exit( &crit_psec, __FILE__, 0);
    }
};

#define PSEC_LOCK_SCOPE() PSEC_Lock __psec_lock_guard__(__LINE__)

/**
 * \brief Macro to set user-enabledness.
 */
#define PSEC_USER_ENA_SET(_port_state_, _user_, _enable_)                \
    do {                                                                 \
        if (_enable_) {                                                  \
            (_port_state_)->if_status.status.users |=  VTSS_BIT(_user_); \
        } else {                                                         \
            (_port_state_)->if_status.status.users &= ~VTSS_BIT(_user_); \
        }                                                                \
    } while (0)

/**
 * \brief Macro to get a user's enabledness.
 */
#define PSEC_USER_ENA_GET(_port_state_, _user_) (((_port_state_)->if_status.status.users & VTSS_BIT(_user_)) != 0)

/**
 * \brief Structure holding both ifindex, isid, and port number
 *
 * Annoyingly enough we get invoked by other user modules with <isid, port>
 * just as we need to call out (both to other user modules and to the MAC module)
 * with <isid, port>, but internally we use ifindex to get and set port state.
 *
 * This is a structure that we carry around between the different functions
 * to ease the conversion job and limit the number of parameters to each function.
 */
typedef struct {
    vtss_isid_t    isid;
    mesa_port_no_t port;
    vtss_ifindex_t ifindex;
} psec_ifindex_t;

/**
 * \brief Internal MAC state
 *
 * Each instance of this structure is used to manage one MAC address.
 *
 * The reason to semi-publicize this structure is to make it available
 * to the vtss::expose interface, so that changes to the public sub-structure
 * (vtss_appl_psec_interface_status_t) can become JSON notifications
 * and SNMP Traps.
 */
typedef struct {
    /**
     * Is TRUE if a hash collision was detected so that this entry
     * could not get added to the MAC table, FALSE otherwise.
     */
    BOOL hw_add_failed;

    /**
     * Is TRUE if the MAC module did not allow us to add this entry
     * (for some unknown reason), FALSE otherwise.
     */
    BOOL sw_add_failed;

    /**
     * Is TRUE if this entry is in the MAC module, FALSE otherwise.
     */
    BOOL in_mac_module;

    /**
     * And it was originally added at this time
     */
    time_t creation_time_secs;

    /**
     * Here is when it was last changed in the MAC table
     */
    time_t changed_time_secs;

    /** During "MAC add" callbacks we let go of the crit that
     * protects our internal state. We use this field to ensure
     * that nothing has happened to the internal state during the
     * callbacks. This number is a unique, ever-increasing
     * number that is only zero when this entry is in the free pool.
     */
    u32 unique;

    /**
     * Forward decision per user module.
     */
    psec_add_method_t forward_decision[VTSS_APPL_PSEC_USER_LAST];

    /**
     * On Linux, each allowed MAC entry must be stored in
     * the kernel's IP filter's allow-list so that it can
     * access the switch's IP stack.
     * This Filter ID is the ID returned by the kernel when
     * an allow-list rule is installed or VTSS_IP_FILTER_ID_NONE
     * if no rule is installed.
     */
    int ip_filter_id_allow_list;

    /**
     * On Linux, each allowed MAC entry must be stored in
     * the kernel's IP filter's deny-list so that it can
     * be specified on which port the MAC entry is allowed.
     * This Filter ID is the ID returned by the kernel when
     * a deny-list rule is installed or VTSS_IP_FILTER_ID_NONE
     * if no rule is installed.
     */
    int ip_filter_id_deny_list;

    /**
     * Public MAC status. This is the publicized MAC status.
     */
    vtss_appl_psec_mac_status_t status;

    /**
     * The FID that this entry is learned on in the MAC table.
     * In case of SVL (Shared VLAN Learning), multiple VIDs may map to the same
     * entry in the MAC table.
     */
    mesa_vid_t fid;
} psec_mac_status_t;

/**
 * Macro to get whether a user is to be called delayed on a MAC-add operation.
 */
#define PSEC_USER_CALL_DELAYED_ADD_GET(_entry_, _user_) \
  ((_entry_)->second.members.users_to_call_delayed_add & VTSS_BIT(_user_)) != 0

/**
 * Macro to get whether a user is to be called delayed on a MAC-del operation.
 */
#define PSEC_USER_CALL_DELAYED_DEL_GET(_entry_, _user_) \
  ((_entry_)->second.members.users_to_call_delayed_del & VTSS_BIT(_user_)) != 0

static  vtss::Map<vtss_appl_psec_mac_map_key_t, psec_mac_status_t> psec_mac_map;
typedef vtss::Map<vtss_appl_psec_mac_map_key_t, psec_mac_status_t>::iterator psec_mac_itr_t;
typedef vtss::Map<vtss_appl_psec_mac_map_key_t, psec_mac_status_t>::const_iterator psec_mac_const_itr_t;

// The following provides inline functions for comparing two psec_semi_public_interface_status_t structures.
VTSS_BASICS_MEMCMP_OPERATOR(psec_semi_public_interface_status_t);

// psec_semi_public_interface_status_t holds the port state available only internally in this module.
// One field in psec_semi_public_interface_status_t, #status, holds the state publically available.
PsecInterfaceStatus psec_semi_public_interface_status("psec_semi_public_interface_status", VTSS_MODULE_ID_PSEC);

// psec_global_notification_status holds global state that one can get notifications on,
// that being SNMP traps or JSON notifications.
// The type it holds is of vtss_appl_psec_global_notification_status_t.
PsecGlobalNotificationStatus psec_global_notification_status;

// The following provides inline functions for comparing two vtss_appl_psec_global_notification_status_t structures.
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_psec_global_notification_status_t);

// psec_interface_notification_status holds the per-interface state that one can get notifications on,
// that being SNMP traps or JSON notifications.
// Each row in this table is a struct of type vtss_appl_psec_interface_notification_status_t.
PsecInterfaceNotificationStatus psec_interface_notification_status("psec_interface_notification_status", VTSS_MODULE_ID_PSEC);

// The following provides inline functions for comparing two vtss_appl_psec_interface_notification_status_t structures.
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_psec_interface_notification_status_t);

/**
 * Wrapper struct that contains the semi-public psec_semi_public_interface_status_t
 * and the public vtss_appl_psec_interface_notification_status_t.
 * Each of these two members are held in its own vtss::expose::TableStatus object.
 */
typedef struct {
    /**
     * Semi-public interface status. This struct has a member (#status) that is public.
     */
    psec_semi_public_interface_status_t if_status;

    /**
     * Interface notification status.
     */
    vtss_appl_psec_interface_notification_status_t notif_status;
} psec_interface_status_t;

/**
 * \brief Reasons for checking whether secure learning with/without CPU copy should be enabled or disabled
 */
typedef enum {
    PSEC_LEARN_CPU_REASON_MAC_ADDRESS_ALLOCATED,
    PSEC_LEARN_CPU_REASON_MAC_ADDRESS_FREED,
    PSEC_LEARN_CPU_REASON_SWITCH_UP,
    PSEC_LEARN_CPU_REASON_OTHER,
} psec_learn_cpu_reason_t;

typedef struct {
    /**
      * The per-user-module aging period measured in seconds.
      * A given module's aging period will only be used if the module is enabled on a given port.
      * A value of 0 means disable aging.
      * Used when an entry is forwarding on a port (i.e. psec_on_mac_add_callback()
      * returned PSEC_ADD_METHOD_FORWARD).
      */
    u32 aging_period_secs[VTSS_APPL_PSEC_USER_LAST];

    /**
      * The per-user-module MAC block period measured in seconds.
      * Used while keeping a non-forwarding entry in the MAC table.
      * A given module's block time will only be used if the module is enabled on a given port.
      * A value of 0 is invalid.
      * Used when an entry is not forwarding on a port (i.e. psec_on_mac_add_callback()
      * returned PSEC_ADD_MEDHOD_BLOCK).
     */
    u32 hold_time_secs[VTSS_APPL_PSEC_USER_LAST];

    /**
      * The per-user-module On-MAC-Add callback function.
      * If non-NULL and the user is enabled on the port on which a MAC
      * address is going to be added, then the callback will be called.
      */
    psec_on_mac_add_callback_f *on_mac_add_callbacks[VTSS_APPL_PSEC_USER_LAST];

    /**
      * The per-user-module On-MAC-Del callback function.
      * If non-NULL and the user is enabled on the port on which a MAC
      * address is going to be deleted, then the callback will be called.
      */
    psec_on_mac_del_callback_f *on_mac_del_callbacks[VTSS_APPL_PSEC_USER_LAST];

    /**
     * Counts the number of yet-to-use MAC entries
     */
    u32 macs_left;
} psec_state_t;

static psec_state_t PSEC_state;

/**
 * \brief Internal MAC state for add and delete callbacks.
 *
 * To avoid deadlocks, we need to keep a separate list of MAC addresses that we
 * need to invoke registered users with when the MAC address is to be deleted.
 * The PSEC_del_callback() function merely registers into this list, and the
 * PSEC thread performs the actual callback.
 */
struct psec_mac_callback_status_t {
    psec_mac_callback_status_t(void)
    {
        // Constructor
        vtss_clear(members);
    }

    struct {
        vtss_isid_t           isid;
        mesa_port_no_t        port;
        mesa_vid_mac_t        vid_mac;
        psec_del_reason_t     reason; // For del-operations only
        psec_add_method_t     forward_decision_add[VTSS_APPL_PSEC_USER_LAST]; // For add-operations only
        psec_add_method_t     forward_decision_del[VTSS_APPL_PSEC_USER_LAST]; // For del-operations only
        vtss_appl_psec_user_t originating_user_add; // For add-operations only
        vtss_appl_psec_user_t originating_user_del; // For del-operations only
        u32                   unique; // For add-operations only

        /**
         * Is TRUE for a given user if that user has not been called back with
         * this MAC address.
         * Use PSEC_USER_CALL_DELAYED_DEL_GET() to get per user.
         */
        u32 users_to_call_delayed_del;

        /**
         * Is TRUE for a given user if that user has not been called back with
         * this MAC address.
         * Use PSEC_USER_CALL_DELAYED_ADD_GET() to get per user.
         */
        u32 users_to_call_delayed_add;
    } members;
};

typedef vtss::Map<vtss_appl_psec_mac_map_key_t, psec_mac_callback_status_t> psec_mac_add_del_map_t;
typedef psec_mac_add_del_map_t::iterator       psec_mac_add_del_itr_t;
static  psec_mac_add_del_map_t                 psec_mac_add_del_map;

// Cached version of msg_switch_exists(). Used to speed up.
// Notice: This is 0-based, so VTSS_ISID_START is at index 0.
BOOL PSEC_switch_exists[VTSS_ISID_CNT];

// Local configuration
static u8                 PSEC_copy_to_primary_switch[VTSS_PORT_BF_SIZE];
static packet_rx_filter_t PSEC_frame_rx_filter;
static void               *PSEC_frame_rx_filter_id = NULL;

/******************************************************************************/
// Thread variables
/******************************************************************************/
static vtss_handle_t PSEC_thread_handle;
static vtss_thread_t PSEC_thread_state;  // Contains space for the scheduler to hold the current thread state.
static vtss_flag_t   PSEC_thread_wait_flag;
#define PSEC_THREAD_WAIT_FLAG_ADD_DEL 0x1

/*lint -save -e19 */
VTSS_ENUM_INC(vtss_appl_psec_user_t);
/*lint -restore */

/******************************************************************************/
// Linux IP filter variables.
/******************************************************************************/
static struct vtss::appl::ip::filter::Owner PSEC_ip_filter_owner = {
    // module_id
    VTSS_MODULE_ID_PSEC,

    // Rule name
    "Port Security",
};

// If not debugging, set PSEC_INLINE to inline
#define PSEC_INLINE inline

/******************************************************************************/
//
// Message handling functions, structures, and state.
//
/******************************************************************************/

/****************************************************************************/
// Message IDs
/****************************************************************************/
typedef enum {
    PSEC_MSG_ID_PRIM_TO_SEC_RATE_LIMIT_CONF, // Tell the secondary switch the current rate-limiter setup.
    PSEC_MSG_ID_PRIM_TO_SEC_PORT_CONF,       // Tell the secondary switch whether to copy frames to the primary switch for one port.
    PSEC_MSG_ID_PRIM_TO_SEC_SWITCH_CONF,     // Tell the secondary switch whether to copy frames to the primary switch for the whole switch.
    PSEC_MSG_ID_SEC_TO_PRIM_FRAME,           // Tell the primary switch the MAC address and VID of a frame received on a Port Security enabled port.
} psec_msg_id_t;

/****************************************************************************/
// Primary->Secondary: Rate limiter setup.
/****************************************************************************/
typedef struct {
    psec_rate_limit_conf_t rate_limit; // Rate limit configuration.
} psec_msg_rate_limit_conf_t;

/****************************************************************************/
// Primary->Secondary: Tell the secondary switch to enable or disable
// registration for frames on a given port. Once set frames will be forwarded to
// the primary switch if enabled.
/****************************************************************************/
typedef struct {
    mesa_port_no_t port;
    BOOL copy_to_primary_switch;
} psec_msg_port_conf_t;

/****************************************************************************/
// Primary->Secondary: Tell the secondary switch on which ports to register for
// frames. Once set, frames will be forwarded to the primary switch if enabled.
/****************************************************************************/
typedef struct {
    u8 copy_to_primary_switch[VTSS_PORT_BF_SIZE];
} psec_msg_switch_conf_t;

/****************************************************************************/
// Secondary->Primary: Whenever any frame is seen on an enabled port, the MAC
// address and VLAN ID along with whether it's a learn frame is sent to
// the primary switch, if there's a reason for it.
/****************************************************************************/
typedef struct {
    mesa_port_no_t port;
    mesa_vid_mac_t vid_mac;
    BOOL           is_learn_frame;
    vtss_isid_t    isid; // It may not always come from the switch that sends this message
} psec_msg_frame_t;

/****************************************************************************/
// Message Identification Header
/****************************************************************************/
typedef struct {
    // Message Version Number
    u32 version; // Set to PSEC_MSG_VERSION

    // Message ID
    psec_msg_id_t msg_id;
} psec_msg_hdr_t;

/****************************************************************************/
// Message.
// This struct contains a union, whose primary purpose is to give the
// size of the biggest of the contained structures.
/****************************************************************************/
typedef struct {
    // Header stuff
    psec_msg_hdr_t hdr;

    // Request message
    union {
        psec_msg_rate_limit_conf_t rate_limit_conf;
        psec_msg_port_conf_t       port_conf;
        psec_msg_switch_conf_t     switch_conf;
        psec_msg_frame_t           frame;
    } u; // Anonymous unions are not allowed in C99 :(
} psec_msg_t;

/****************************************************************************/
// Frame Message
// This struct is needed because transmission of learn frames is not done
// using the normal buffer alloc structure, because that may cause a deadlock
// since its done from the Packet Rx thread.
/****************************************************************************/
typedef struct {
    psec_msg_hdr_t   hdr;
    psec_msg_frame_t frame;
} psec_msg_hdr_and_frame_t;

static void *PSEC_msg_buf_pool; // Static, mutex-protected message transmission buffer(s).

// Current version of Port Security messages (1-based).
// Future revisions of this module should support previous versions if applicable.
#define PSEC_MSG_VERSION 1

#define PSEC_MAC_STR_BUF_SIZE 70

/******************************************************************************/
// PSEC_mac_conf_to_str()
/******************************************************************************/
static const char *PSEC_mac_conf_to_str(vtss_ifindex_t ifindex, const vtss_appl_psec_mac_conf_t *const mac_conf, char buf[PSEC_MAC_STR_BUF_SIZE])
{
    char mac_str[18];
    sprintf(buf, "<Interface, VID, MAC> = <%u, %u, %s>", VTSS_IFINDEX_PRINTF_ARG(ifindex), mac_conf->vlan, misc_mac_txt(mac_conf->mac.addr, mac_str));
    return buf;
}

/******************************************************************************/
// PSEC_vid_mac_to_str()
/******************************************************************************/
static const char *PSEC_vid_mac_to_str(vtss_ifindex_t ifindex, const mesa_vid_mac_t *const vid_mac, char buf[PSEC_MAC_STR_BUF_SIZE])
{
    char mac_str[18];
    sprintf(buf, "<Interface, VID, MAC> = <%u, %u, %s>", VTSS_IFINDEX_PRINTF_ARG(ifindex), vid_mac->vid, misc_mac_txt(vid_mac->mac.addr, mac_str));
    return buf;
}

/******************************************************************************/
// PSEC_mac_itr_to_str()
/******************************************************************************/
static const char *PSEC_mac_itr_to_str(psec_mac_const_itr_t mac_itr, char buf[PSEC_MAC_STR_BUF_SIZE])
{
    char mac_str[18];
    sprintf(buf, "<Interface, VID, MAC> = <%u, %u, %s>", VTSS_IFINDEX_PRINTF_ARG(mac_itr->first.ifindex), mac_itr->second.status.vid_mac.vid, misc_mac_txt(mac_itr->second.status.vid_mac.mac.addr, mac_str));
    return buf;
}

/******************************************************************************/
// vtss_appl_psec_mac_map_key_t::operator<
/******************************************************************************/
bool operator<(const vtss_appl_psec_mac_map_key_t &lhs, const vtss_appl_psec_mac_map_key_t &rhs)
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

/****************************************************************************/
// PSEC_vid_mac_to_mac_conf()
/****************************************************************************/
static void PSEC_vid_mac_to_mac_conf(const mesa_vid_mac_t *const vid_mac, vtss_appl_psec_mac_conf_t *mac_conf, vtss_appl_psec_mac_type_t mac_type)
{
    memset(mac_conf, 0, sizeof(*mac_conf));
    mac_conf->vlan     = vid_mac->vid;
    mac_conf->mac      = vid_mac->mac;
    mac_conf->mac_type = mac_type;
}

/****************************************************************************/
// PSEC_mac_conf_to_vid_mac()
/****************************************************************************/
static void PSEC_mac_conf_to_vid_mac(const vtss_appl_psec_mac_conf_t *const mac_conf, mesa_vid_mac_t *vid_mac)
{
    memset(vid_mac, 0, sizeof(*vid_mac));
    vid_mac->vid = mac_conf->vlan;
    vid_mac->mac = mac_conf->mac;
}

/******************************************************************************/
// PSEC_ifindex_from_port()
/******************************************************************************/
static mesa_rc PSEC_ifindex_from_port(vtss_isid_t isid, mesa_port_no_t port, psec_ifindex_t *psec_ifindex, int line_no)
{
    if (vtss_ifindex_from_port(isid, port, &psec_ifindex->ifindex) != VTSS_RC_OK) {
        T_E("Line %d: Unable to convert <isid, port> = <%u, %u> to ifindex", line_no, isid, port);
        // Our return value is better than vtss_ifindex_from_port()'s.
        return VTSS_APPL_PSEC_RC_INV_PORT;
    }

    psec_ifindex->isid = isid;
    psec_ifindex->port = port;

    return VTSS_RC_OK;
}

/******************************************************************************/
// PSEC_ifindex_from_ifindex()
/******************************************************************************/
static mesa_rc PSEC_ifindex_from_ifindex(vtss_ifindex_t ifindex, psec_ifindex_t *psec_ifindex, int line_no, BOOL give_error = TRUE)
{
    vtss_ifindex_elm_t ife;

    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK) {
        if (give_error) {
            T_E("Line %d: Unable to decompose ifindex = %u", line_no, VTSS_IFINDEX_PRINTF_ARG(ifindex));
        }

        return VTSS_APPL_PSEC_RC_INV_IFINDEX;
    }

    if (ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
        if (give_error) {
            T_E("Line %d: ifindex = %u is not a port type", line_no, VTSS_IFINDEX_PRINTF_ARG(ifindex));
        }

        return VTSS_APPL_PSEC_RC_IFINDEX_NOT_REPRESENTING_A_PORT;
    }

    psec_ifindex->isid = ife.isid;
    psec_ifindex->port = ife.ordinal;
    psec_ifindex->ifindex = ifindex;

    return VTSS_RC_OK;
}

/******************************************************************************/
// PSEC_macs_left_set()
/******************************************************************************/
static void PSEC_macs_left_set(BOOL increment)
{
    vtss_appl_psec_global_notification_status_t global_notif_status;

    if (increment) {
        PSEC_state.macs_left++;
    } else {
        PSEC_state.macs_left--;
    }

    if (psec_global_notification_status.get(&global_notif_status) != VTSS_RC_OK) {
        T_E("Unable to get global notification status");
        memset(&global_notif_status, 0, sizeof(global_notif_status));
    }

    global_notif_status.pool_depleted = PSEC_state.macs_left == 0;

    if (psec_global_notification_status.set(&global_notif_status) != VTSS_RC_OK) {
        T_E("Unable to set global notification status");
    }
}

/******************************************************************************/
// PSEC_interface_status_get()
/******************************************************************************/
static mesa_rc PSEC_interface_status_get(vtss_ifindex_t ifindex, psec_interface_status_t *port_state, int line_no)
{
    if (psec_semi_public_interface_status.get(ifindex, &port_state->if_status) != VTSS_RC_OK) {
        T_E("Line %d: Unable to obtain interface status for ifindex = %u", line_no, VTSS_IFINDEX_PRINTF_ARG(ifindex));
        return VTSS_APPL_PSEC_RC_INV_PORT;
    }

    if (psec_interface_notification_status.get(ifindex, &port_state->notif_status) != VTSS_RC_OK) {
        T_E("Line %d: Unable to obtain interface notification status for ifindex = %u", line_no, VTSS_IFINDEX_PRINTF_ARG(ifindex));
        return VTSS_APPL_PSEC_RC_INV_PORT;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// PSEC_interface_status_set()
/******************************************************************************/
static mesa_rc PSEC_interface_status_set(vtss_ifindex_t ifindex, psec_interface_status_t *port_state, int line_no)
{
    if (psec_semi_public_interface_status.set(ifindex, &port_state->if_status) != VTSS_RC_OK) {
        T_E("Line %d: Unable to set interface status for ifindex = %u", line_no, VTSS_IFINDEX_PRINTF_ARG(ifindex));
        return VTSS_APPL_PSEC_RC_INV_PORT;
    }

    if (psec_interface_notification_status.set(ifindex, &port_state->notif_status) != VTSS_RC_OK) {
        T_E("Line %d: Unable to set interface notification status for ifindex = %u", line_no, VTSS_IFINDEX_PRINTF_ARG(ifindex));
        return VTSS_APPL_PSEC_RC_INV_PORT;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// PSEC_mac_itr_get()
/******************************************************************************/
static psec_mac_itr_t PSEC_mac_itr_get(const vtss_appl_psec_mac_map_key_t *key)
{
    return psec_mac_map.find(*key);
}

/******************************************************************************/
// PSEC_mac_itr_get_first_from_ifindex()
/******************************************************************************/
static psec_mac_itr_t PSEC_mac_itr_get_first_from_ifindex(vtss_ifindex_t ifindex)
{
    psec_mac_itr_t mac_itr;

    for (mac_itr = psec_mac_map.begin(); mac_itr != psec_mac_map.end(); ++mac_itr) {
        if (mac_itr->first.ifindex == ifindex) {
            return mac_itr;
        } else if (mac_itr->first.ifindex > ifindex) {
            // No need to look further because the map is sorted by ifindex as first key.
            break;
        }
    }

    return psec_mac_map.end();
}

/******************************************************************************/
// PSEC_mac_itr_get_next_from_ifindex()
// Returns an iterator to a MAC entry that has an ifindex which is higher than
// #ifindex
/******************************************************************************/
static psec_mac_itr_t PSEC_mac_itr_get_next_from_ifindex(vtss_ifindex_t ifindex)
{
    psec_mac_itr_t mac_itr;

    for (mac_itr = psec_mac_map.begin(); mac_itr != psec_mac_map.end(); ++mac_itr) {
        if (mac_itr->first.ifindex > ifindex) {
            return mac_itr;
        }
    }

    return psec_mac_map.end();
}

/******************************************************************************/
// PSEC_mac_itr_get_first()
/******************************************************************************/
static psec_mac_itr_t PSEC_mac_itr_get_first(void)
{
    return psec_mac_map.begin();
}

/******************************************************************************/
// PSEC_mac_itr_get_from_fid_mac()
/******************************************************************************/
static psec_mac_itr_t PSEC_mac_itr_get_from_fid_mac(mesa_vid_t fid, const mesa_mac_t *mac)
{
    psec_mac_itr_t mac_itr = psec_mac_map.begin();

    for (mac_itr = psec_mac_map.begin(); mac_itr != psec_mac_map.end(); ++mac_itr) {
        if (mac_itr->second.fid == fid && memcmp(&mac_itr->first.mac, mac, sizeof(mac_itr->first.mac)) == 0) {
            return mac_itr;
        }
    }

    return psec_mac_map.end();
}

/******************************************************************************/
// PSEC_mac_itr_find_current_or_next_in_mac_module()
/******************************************************************************/
static psec_mac_itr_t PSEC_mac_itr_find_current_or_next_in_mac_module(psec_mac_itr_t mac_itr)
{
    while (mac_itr != psec_mac_map.end() && (mac_itr->second.hw_add_failed || mac_itr->second.sw_add_failed)) {
        mac_itr++;
    }

    return mac_itr;
}

/******************************************************************************/
// PSEC_mac_itr_alloc()
/******************************************************************************/
static psec_mac_itr_t PSEC_mac_itr_alloc(const vtss_appl_psec_mac_map_key_t *key)
{
    static u32 unique;
    psec_mac_itr_t mac_itr;

    if (PSEC_state.macs_left == 0) {
        // Out of Port Security-controlled MAC entries.
        return psec_mac_map.end();
    }

    // The .get() method allocates if it can't find an existing (which it shouldn't
    // be able to, given that we are about to allocate a new).
    mac_itr = psec_mac_map.get(*key);
    if (mac_itr != psec_mac_map.end()) {
        if (++unique == 0) {
            // 0 is reserved for a non-allocated item.
            ++unique;
        }

        memset(&mac_itr->second, 0, sizeof(mac_itr->second));
        PSEC_macs_left_set(FALSE /* decrement */);
        mac_itr->second.status.ifindex          = key->ifindex;
        mac_itr->second.status.vid_mac.vid      = key->vlan;
        mac_itr->second.status.vid_mac.mac      = key->mac;
        mac_itr->second.creation_time_secs      = msg_uptime_get(VTSS_ISID_LOCAL);
        mac_itr->second.unique                  = unique;
        mac_itr->second.ip_filter_id_deny_list  = VTSS_IP_FILTER_ID_NONE;
        mac_itr->second.ip_filter_id_allow_list = VTSS_IP_FILTER_ID_NONE;
        mac_itr->second.fid                     = psec_limit_mgmt_fid_get(key->vlan);
    }

    return mac_itr;
}

/******************************************************************************/
// PSEC_mac_itr_free()
/******************************************************************************/
static void PSEC_mac_itr_free(psec_mac_itr_t mac_itr)
{
    mac_itr->second.unique = 0;
    psec_mac_map.erase(mac_itr);

    // One more entry to use
    PSEC_macs_left_set(TRUE /* increment */);
}

/******************************************************************************/
// PSEC_msg_buf_alloc()
// Blocks until a buffer is available, then takes and returns it.
/******************************************************************************/
static psec_msg_t *PSEC_msg_buf_alloc(psec_msg_id_t msg_id)
{
    psec_msg_t *msg = (psec_msg_t *)msg_buf_pool_get(PSEC_msg_buf_pool);
    VTSS_ASSERT(msg);
    msg->hdr.version = PSEC_MSG_VERSION;
    msg->hdr.msg_id = msg_id;
    return msg;
}

/******************************************************************************/
// PSEC_msg_tx_done()
// Called when message is successfully or unsuccessfully transmitted.
/******************************************************************************/
static void PSEC_msg_tx_done(void *contxt, void *msg, msg_tx_rc_t rc)
{
    // Release the message back to the message buffer pool
    (void)msg_buf_pool_put(msg);
}

/******************************************************************************/
// PSEC_msg_tx()
// Do transmit a message.
/******************************************************************************/
static void PSEC_msg_tx(psec_msg_t *msg, vtss_isid_t isid, size_t len)
{
    msg_tx_adv(NULL, PSEC_msg_tx_done, MSG_TX_OPT_DONT_FREE, VTSS_MODULE_ID_PSEC, isid, msg, len + MSG_TX_DATA_HDR_LEN(psec_msg_t, u));
}

/******************************************************************************/
// psec_msg_tx_rate_limit_conf()
/******************************************************************************/
void psec_msg_tx_rate_limit_conf(vtss_isid_t isid, psec_rate_limit_conf_t *conf)
{
    psec_msg_t *msg;

    if (!msg_switch_exists(isid)) {
        return;
    }

    // Get a buffer.
    msg = PSEC_msg_buf_alloc(PSEC_MSG_ID_PRIM_TO_SEC_RATE_LIMIT_CONF);

    // Copy conf to buffer
    msg->u.rate_limit_conf.rate_limit = *conf;

    T_D("Transmitting rate-limit conf to isid=%d", isid);

    // Transmit it.
    PSEC_msg_tx(msg, isid, sizeof(msg->u.rate_limit_conf));
}

/******************************************************************************/
// PSEC_msg_tx_port_conf()
// Transmit enabledness for @port to @isid.
/******************************************************************************/
static void PSEC_msg_tx_port_conf(vtss_isid_t isid, mesa_port_no_t port, BOOL enable)
{
    psec_msg_t *msg;

    if (!msg_switch_exists(isid)) {
        return;
    }

    // Get a buffer.
    msg = PSEC_msg_buf_alloc(PSEC_MSG_ID_PRIM_TO_SEC_PORT_CONF);

    // Copy conf to buffer
    msg->u.port_conf.port           = port;
    msg->u.port_conf.copy_to_primary_switch = enable;

    T_D("Tx port conf to isid:port:ena=%d:%d:%d", isid, port, enable);

    // Transmit it.
    PSEC_msg_tx(msg, isid, sizeof(msg->u.port_conf));
}

/******************************************************************************/
// PSEC_msg_tx_switch_conf()
// Transmit enabledness for all ports on switch @isid to @isid.
/******************************************************************************/
static PSEC_INLINE void PSEC_msg_tx_switch_conf(vtss_isid_t isid)
{
    psec_msg_t     *msg;
    port_iter_t    pit;
    psec_ifindex_t psec_ifindex;

    if (port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL) != VTSS_RC_OK) {
        // Don't wanna allocate a message buffer if the switch doesn't exist.
        return;
    }

    // Get a buffer.
    msg = PSEC_msg_buf_alloc(PSEC_MSG_ID_PRIM_TO_SEC_SWITCH_CONF);

    // Copy conf to buffer
    VTSS_PORT_BF_CLR(msg->u.switch_conf.copy_to_primary_switch);
    while (port_iter_getnext(&pit)) {
        psec_interface_status_t port_state;

        if (PSEC_ifindex_from_port(isid, pit.iport, &psec_ifindex, __LINE__) != VTSS_RC_OK) {
            continue;
        }

        if (PSEC_interface_status_get(psec_ifindex.ifindex, &port_state, __LINE__) != VTSS_RC_OK) {
            continue;
        }

        if (port_state.if_status.status.users) {
            // At least one user is enabled on this port.
            VTSS_PORT_BF_SET(msg->u.switch_conf.copy_to_primary_switch, pit.iport, 1);
        }
    }

    T_D("Tx switch conf to isid %d", isid);

    // Transmit it.
    PSEC_msg_tx(msg, isid, sizeof(msg->u.switch_conf));
}

/******************************************************************************/
// PSEC_msg_tx_frame()
// Transmit (learn) frame properties to current primary switch
/******************************************************************************/
static PSEC_INLINE void PSEC_msg_tx_frame(vtss_isid_t originating_isid, mesa_port_no_t port, mesa_vid_mac_t *vid_mac, BOOL learn_flag)
{
    psec_msg_hdr_and_frame_t *msg;

    // Do not wait for buffers here, since that may lead to a deadlock, causing
    // the msg module not to receive MACKs on the primary switch, because this
    // is called from the packet rx thread, and holding up that thread causes
    // the message module to go dead.

    // The msg is freed by the message module
    VTSS_MALLOC_CAST(msg, sizeof(*msg));

    if (!msg) {
        T_W("Unable to allocate %zu bytes for learn frame", sizeof(*msg));
        return;
    }

    msg->hdr.version          = PSEC_MSG_VERSION;
    msg->hdr.msg_id           = PSEC_MSG_ID_SEC_TO_PRIM_FRAME;
    msg->frame.port           = port;
    msg->frame.vid_mac        = *vid_mac;
    msg->frame.is_learn_frame = learn_flag;
    msg->frame.isid           = originating_isid;

    // Let the message module free the buffer.
    // These frames are subject to shaping.
    msg_tx_adv(NULL, NULL, (msg_tx_opt_t)(MSG_TX_OPT_NO_ALLOC_ON_LOOPBACK | MSG_TX_OPT_SHAPE), VTSS_MODULE_ID_PSEC, 0, msg, sizeof(*msg));
}

/******************************************************************************/
// PSEC_ip_filter_port_update()
// Not stack-aware.
// This is the first filter an IP frame goes through. If a given port is port-
// security enabled, it will be added to the deny-list with an action of
// checking the allow-list. The allow-list contains per-MAC-address entries.
/******************************************************************************/
static void PSEC_INLINE PSEC_ip_filter_port_update(mesa_port_no_t port, BOOL enable)
{
    static vtss::appl::ip::filter::PortMask port_mask;
    static int                              ip_filter_id = VTSS_IP_FILTER_ID_NONE;
    vtss::appl::ip::filter::PortMask        new_port_mask;
    vtss::appl::ip::filter::Rule            rule;
    mesa_rc                                 rc;

    T_DG(TRACE_GRP_IP_FILTER, "Enter: port = %u, enable = %d", port, enable);

    new_port_mask = port_mask;
    new_port_mask.set(port, enable);

    if (new_port_mask == port_mask) {
        // No changes.
        T_DG(TRACE_GRP_IP_FILTER, "new_port_mask == port_mask == 0b%s", new_port_mask.to_string().c_str());
        return;
    }

    rule.emplace_back(vtss::appl::ip::filter::element_port_mask(new_port_mask));

    T_DG(TRACE_GRP_IP_FILTER, "Current filter ID = %d, current port mask = 0b%s, new port mask = 0b%s", ip_filter_id, port_mask.to_string().c_str(), new_port_mask.to_string().c_str());

    if (port_mask.any() && new_port_mask.any()) {
        // Change current.
        T_DG(TRACE_GRP_IP_FILTER, "Changing current deny-list rule");
        if ((rc = vtss::appl::ip::filter::rule_update(ip_filter_id, rule)) != VTSS_RC_OK) {
            T_EG(TRACE_GRP_IP_FILTER, "Unable to update existing deny-list rule. Error = %s", error_txt(rc));
        }
    }  else if (port_mask.any()) {
        // Remove rule.
        T_DG(TRACE_GRP_IP_FILTER, "Removing deny-list rule");
        if ((rc = vtss::appl::ip::filter::rule_del(ip_filter_id)) != VTSS_RC_OK) {
            T_EG(TRACE_GRP_IP_FILTER, "Unable to delete deny-list rule. Error = %s", error_txt(rc));
        }

        ip_filter_id = VTSS_IP_FILTER_ID_NONE;
    } else {
        // Add new rule.
        if ((rc = vtss::appl::ip::filter::deny_list_rule_add(&ip_filter_id, &PSEC_ip_filter_owner, rule, vtss::appl::ip::filter::Action::check_allow_list)) != VTSS_RC_OK) {
            T_EG(TRACE_GRP_IP_FILTER, "Unable to add deny-list rule. Error = %s", error_txt(rc));
        } else {
            T_DG(TRACE_GRP_IP_FILTER, "Added new deny-list rule and got filter ID = %d", ip_filter_id);
        }
    }

    port_mask = new_port_mask;
}

/******************************************************************************/
// PSEC_ip_filter_rule_del()
/******************************************************************************/
static void PSEC_ip_filter_rule_del(int *ip_filter_id, const char *color_of_list)
{
    mesa_rc rc;

    if (*ip_filter_id != VTSS_IP_FILTER_ID_NONE) {
        T_DG(TRACE_GRP_IP_FILTER, "Removing %s-list rule", color_of_list);

        if ((rc = vtss::appl::ip::filter::rule_del(*ip_filter_id)) != VTSS_RC_OK) {
            T_EG(TRACE_GRP_IP_FILTER, "Unable to delete %s-list rule. Error = %s", color_of_list, error_txt(rc));
        }

        *ip_filter_id = VTSS_IP_FILTER_ID_NONE;
    } else {
        T_DG(TRACE_GRP_IP_FILTER, "No %s-list rule installed, so nothing to be removed", color_of_list);
    }
}

/******************************************************************************/
// PSEC_ip_filter_mac_update()
// Not stack-aware.
// When a MAC address is forwarding, it will be added twice in the IP filter
// lists:
//   Once in the first list that is checked. It is added with a rule that denies
//   it on all ports except the source port for the MAC.
//   The MAC address is also added to the allow-list. This ensures that the
//   port-specific rule added in PSEC_ip_filter_mac_update() with an action
//   of "check-allow-list" will allow the MAC address.
// When a MAC address is blocking, it is added to the deny-list with action
// drop on all ports, but removed from the allow-list.
// Finally, when the MAC address is to be totally removed from the MAC table
// it will also be totally removed from both the deny- and the allow-list.
/******************************************************************************/
static void PSEC_INLINE PSEC_ip_filter_mac_update(mesa_port_no_t port, psec_mac_itr_t mac_itr)
{
    vtss::appl::ip::filter::Rule rule;
    char                         buf[PSEC_MAC_STR_BUF_SIZE];
    mesa_rc                      rc;

    (void)PSEC_mac_itr_to_str(mac_itr, buf);
    T_IG(TRACE_GRP_IP_FILTER, "%s: Current deny-list filter ID = %d, current allow-list filter ID = %d, delete = %s, block = %s",
         buf,
         mac_itr->second.ip_filter_id_deny_list,
         mac_itr->second.ip_filter_id_allow_list,
         port == VTSS_PORT_NO_NONE ? "Yes" : "No",
         mac_itr->second.status.blocked ? "Yes" : "No");

    if (port == VTSS_PORT_NO_NONE) {
        // Remove it from both the deny- and the allow-list.
        PSEC_ip_filter_rule_del(&mac_itr->second.ip_filter_id_deny_list, "deny");
        PSEC_ip_filter_rule_del(&mac_itr->second.ip_filter_id_allow_list, "allow");
    } else if (mac_itr->second.status.blocked) {
        // Delete the MAC entry from the allow-list, if it's there.
        PSEC_ip_filter_rule_del(&mac_itr->second.ip_filter_id_allow_list, "allow");

        // Deny-list addition is done further down this function.
    } else {
        // The entry is forwarding. Add it to both the deny-list and the allow-
        // list (deny-list addition is done further down this function).

        // It must be added to the allow-list, because we have a port-based rule
        // in the deny-list for a port-security enabled port, that asks the
        // filter to check the allow-list. If it wasn't in the allow-list, it
        // would not allow to forward.
        if (mac_itr->second.ip_filter_id_allow_list == VTSS_IP_FILTER_ID_NONE) {
            rule.emplace_back(vtss::appl::ip::filter::element_mac_src(mac_itr->second.status.vid_mac.mac));
            rule.emplace_back(vtss::appl::ip::filter::element_vlan(mac_itr->second.status.vid_mac.vid));

            if ((rc = vtss::appl::ip::filter::allow_list_rule_add(&mac_itr->second.ip_filter_id_allow_list, &PSEC_ip_filter_owner, rule)) != VTSS_RC_OK) {
                T_EG(TRACE_GRP_IP_FILTER, "%s: Unable to add allow-list rule. Error = %s", buf, error_txt(rc));
            } else {
                T_DG(TRACE_GRP_IP_FILTER, "%s: Added allow-list rule and got filter ID = %d", buf, mac_itr->second.ip_filter_id_allow_list);
            }
        } else {
            T_DG(TRACE_GRP_IP_FILTER, "%s: Allow-list rule already installed", buf);
        }
    }

    if (port != VTSS_PORT_NO_NONE) {
        vtss::appl::ip::filter::PortMask port_mask;

        // If a MAC address is in the MAC table, there must also be a
        // corresponding deny-list rule. If the MAC address is allowed to
        // forward, then it is also allowed to forward into the IP stack if and
        // only if it is received on the port on which it is allowed to forward,
        // so we control the deny-list rule's forwardness by setting the
        // port-mask to all-ones, and if forwarding, clearing the MAC address'
        // source port bit. This will cause frames from non-source-ports to be
        // discarded.
        port_mask.set(); // Sets all bits in port_mask.
        if (!mac_itr->second.status.blocked) {
            psec_ifindex_t psec_ifindex;

            if (PSEC_ifindex_from_ifindex(mac_itr->first.ifindex, &psec_ifindex, __LINE__) != VTSS_RC_OK) {
                return;
            }

            // Allow the corresponding port to forward into IP stack.
            port_mask &= ~VTSS_BIT64(psec_ifindex.port);
        }

        rule.clear();
        rule.emplace_back(vtss::appl::ip::filter::element_mac_src(mac_itr->second.status.vid_mac.mac));
        rule.emplace_back(vtss::appl::ip::filter::element_vlan(mac_itr->second.status.vid_mac.vid));
        rule.emplace_back(vtss::appl::ip::filter::element_port_mask(port_mask));

        if (mac_itr->second.ip_filter_id_deny_list == VTSS_IP_FILTER_ID_NONE) {
            // Such a rule doesn't exist. Create it.
            if ((rc = vtss::appl::ip::filter::deny_list_rule_add(&mac_itr->second.ip_filter_id_deny_list, &PSEC_ip_filter_owner, rule, vtss::appl::ip::filter::Action::drop)) != VTSS_RC_OK) {
                T_EG(TRACE_GRP_IP_FILTER, "%s: Unable to add deny-list rule. Error = %s", buf, error_txt(rc));
            } else {
                T_DG(TRACE_GRP_IP_FILTER, "%s: Added deny-list rule and got filter ID = %d", buf, mac_itr->second.ip_filter_id_deny_list);
            }
        } else {
            // Change current.
            if ((rc = vtss::appl::ip::filter::rule_update(mac_itr->second.ip_filter_id_deny_list, rule)) != VTSS_RC_OK) {
                T_EG(TRACE_GRP_IP_FILTER, "%s: Unable to update existing deny-list rule. Error = %s", buf, error_txt(rc));
            } else {
                T_DG(TRACE_GRP_IP_FILTER, "%s: Changing current deny-list rule", buf);
            }
        }
    }
}

/******************************************************************************/
// PSEC_mesa_mac_entry_init()
/******************************************************************************/
static void PSEC_mesa_mac_entry_init(mesa_mac_table_entry_t &entry, psec_mac_itr_t mac_itr, mesa_port_no_t port)
{
    vtss_clear(entry);
    entry.vid_mac.mac       = mac_itr->second.status.vid_mac.mac;
    entry.vid_mac.vid       = mac_itr->second.fid;
    entry.copy_to_cpu_smac  = mac_itr->second.status.cpu_copying;
    entry.locked            = 1;
    entry.cpu_queue         = PACKET_XTR_QU_MAC;
    mac_itr->second.changed_time_secs = msg_uptime_get(VTSS_ISID_LOCAL);

    if (!mac_itr->second.status.blocked) {
        entry.destination[port] = 1;
    }
}

/******************************************************************************/
// PSEC_mac_module_chg()
// Ask the MAC module to add a new entry (or change an existing).
/******************************************************************************/
static BOOL PSEC_mac_module_chg(psec_ifindex_t *psec_ifindex, psec_mac_itr_t mac_itr, int called_from)
{
    char                   buf[PSEC_MAC_STR_BUF_SIZE];
    mesa_mac_table_entry_t entry;
    mesa_rc                rc;

    (void)PSEC_mac_itr_to_str(mac_itr, buf);

    T_DG(TRACE_GRP_MAC_MODULE, "%s: MAC Add/Chg. FID = %u. Called from line %d", buf, mac_itr->second.fid, called_from);

    PSEC_ip_filter_mac_update(psec_ifindex->port, mac_itr);

    PSEC_mesa_mac_entry_init(entry, mac_itr, psec_ifindex->port);

    if ((rc = mesa_mac_table_add(NULL, &entry)) != VTSS_RC_OK) {
        mac_itr->second.in_mac_module = FALSE;
        T_DG(TRACE_GRP_MAC_MODULE, "%s: MAC Add failed. Called from line %d. Error = %s", buf, called_from, error_txt(rc));
    } else {
        mac_itr->second.in_mac_module = TRUE;
    }

    return rc == VTSS_RC_OK;
}

/******************************************************************************/
// PSEC_mac_module_del()
/******************************************************************************/
static void PSEC_mac_module_del(psec_ifindex_t *psec_ifindex, psec_mac_itr_t mac_itr)
{
    PSEC_ip_filter_mac_update(VTSS_PORT_NO_NONE, mac_itr);

    // Only do this if the entry is known to be in the MAC table.
    if (mac_itr->second.in_mac_module) {
        char           buf[PSEC_MAC_STR_BUF_SIZE];
        mesa_vid_t     fid     = mac_itr->second.fid;
        mesa_vid_mac_t vid_mac = mac_itr->second.status.vid_mac;
        mesa_rc        rc;

        vid_mac.vid = fid;

        (void)PSEC_mac_itr_to_str(mac_itr, buf);

        // Unregister volatile MAC entry from the MAC table
        T_DG(TRACE_GRP_MAC_MODULE, "%s: Unregistering MAC address on FID = %u", buf, fid);

        if ((rc = mesa_mac_table_del(NULL, &vid_mac)) != VTSS_RC_OK) {
            // When going from primary switch to secondary switch, the MAC_ERROR_STACK_STATE is very likely to be returned.
            T_WG(TRACE_GRP_MAC_MODULE, "%s: mesa_mac_table_del() failed (rc = %s)", buf, error_txt(rc));
        }

        // Now it's no longer there.
        mac_itr->second.in_mac_module = FALSE;
    }
}

/******************************************************************************/
// PSEC_mac_module_sec_learn_cpu_copy()
//
// Learn mode must still use the MAC module and not go directly to the API,
// because the learn-mode may be changed through the MAC module directly by
// management functions as well as by us.
/******************************************************************************/
static PSEC_INLINE void PSEC_mac_module_sec_learn_cpu_copy(psec_ifindex_t *psec_ifindex, psec_interface_status_t *port_state, BOOL enable, BOOL cpu_copy)
{
    mesa_rc                 rc;

    T_IG(TRACE_GRP_MAC_MODULE, "Setting %u:%u to SEC_LEARN = %d, CPU_COPY = %d", psec_ifindex->isid, psec_ifindex->port, enable, cpu_copy);

    PSEC_ip_filter_port_update(psec_ifindex->port, enable);


    if (enable) {
        port_state->if_status.status.sec_learning = TRUE;
        port_state->if_status.status.cpu_copying  = cpu_copy;

        if ((rc = mac_mgmt_learn_mode_force_secure(psec_ifindex->isid, psec_ifindex->port, cpu_copy)) != VTSS_RC_OK) {
            T_WG(TRACE_GRP_MAC_MODULE, "mac_mgmt_learn_mode_force_secure() failed (error=%s)", error_txt(rc));
        }
    } else {
        port_state->if_status.status.sec_learning = FALSE;
        port_state->if_status.status.cpu_copying  = FALSE; // Superfluous
        if ((rc = mac_mgmt_learn_mode_revert(psec_ifindex->isid, psec_ifindex->port)) != VTSS_RC_OK) {
            T_WG(TRACE_GRP_MAC_MODULE, "mac_mgmt_learn_mode_revert() failed (error=%s)", error_txt(rc));
        }
    }
}

/******************************************************************************/
// PSEC_sec_learn_cpu_copy_check()
// Reason can be one of the following:
//   PSEC_LEARN_CPU_REASON_MAC_ADDRESS_ALLOCATED
//   PSEC_LEARN_CPU_REASON_MAC_ADDRESS_FREED
//   PSEC_LEARN_CPU_REASON_SWITCH_UP
//   PSEC_LEARN_CPU_REASON_OTHER
//
// Notice: This function possibly reads and writes psec_interface_status_t for
//         all ports, so it must be saved by caller prior to invoking it.
/******************************************************************************/
static void PSEC_sec_learn_cpu_copy_check(const psec_ifindex_t *const psec_ifindex, psec_interface_status_t *port_state, psec_learn_cpu_reason_t reason, int called_from)
{
    vtss_isid_t    isid_iter, isid_start, isid_end;
    mesa_port_no_t port_iter, port_start, port_end;

    T_DG(TRACE_GRP_MAC_MODULE, "%u:%u: Reason = %d, called from line %d", psec_ifindex->isid, psec_ifindex->port, reason, called_from);

    // We use isid_start, isid_end, port_start, and port_end as 0-based in this function.
    switch (reason) {
    case PSEC_LEARN_CPU_REASON_MAC_ADDRESS_ALLOCATED:
        // In this case, it might be that we have run out of MAC addresses by
        // this new allocation. This will affect all ports in the stack.
        if (PSEC_state.macs_left == 0) {
            // We did run out
            isid_start = 0;
            isid_end   = VTSS_ISID_CNT - 1;
            port_start = 0;
            port_end   = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) - 1;
        } else {
            // We didn't. Then it only affects this port.
            isid_start = isid_end = psec_ifindex->isid - VTSS_ISID_START;
            port_start = port_end = psec_ifindex->port;
        }
        break;

    case PSEC_LEARN_CPU_REASON_MAC_ADDRESS_FREED:
        // In this case, it might be that the recently freed MAC address entry
        // caused the free list to go from empty to non-empty.
        if (PSEC_state.macs_left == 1) {
            // Got a new entry.
            isid_start = 0;
            isid_end   = VTSS_ISID_CNT - 1;
            port_start = 0;
            port_end   = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) - 1;
        } else {
            // We didn't. Then it only affects this port.
            isid_start = isid_end = psec_ifindex->isid - VTSS_ISID_START;
            port_start = port_end = psec_ifindex->port;
        }
        break;

    case PSEC_LEARN_CPU_REASON_SWITCH_UP:
        // In this case we need to check the switch in question and (un)register
        // all currently registered secure learnings.
        isid_start = isid_end = psec_ifindex->isid - VTSS_ISID_START;
        port_start = 0;
        port_end   = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) - 1;
        break;

    case PSEC_LEARN_CPU_REASON_OTHER:
        // Only check this isid:port.
        isid_start = isid_end = psec_ifindex->isid - VTSS_ISID_START;
        port_start = port_end = psec_ifindex->port;
        break;

    default:
        T_E("Unknown reason (%d)", reason);
        return;
    }

    // Loop through all the switches and ports we need to check
    for (isid_iter = isid_start; isid_iter <= isid_end; isid_iter++) {
        // The number of ports on the switch indicated by isid_iter may not be
        // fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT), because the same binary may fit multiple SKUs with
        // different port counts. If we are asked to iterate to the end,
        // pick the actual port count on that switch.
        mesa_port_no_t port_end2 = port_end == fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) - 1 ? port_count_max() : port_end;

        for (port_iter = port_start; port_iter <= port_end2; port_iter++) {
            psec_interface_status_t port_state2, *new_port_state;
            psec_ifindex_t          psec_ifindex2;
            BOOL                    new_enable_secure_learning;
            BOOL                    new_enable_cpu_copying;
            BOOL                    old_enable_secure_learning;
            BOOL                    old_enable_cpu_copying;

            if (port_iter >= port_count_max()) {
                continue;
            }

            if (PSEC_ifindex_from_port(isid_iter + VTSS_ISID_START, port_iter, &psec_ifindex2, __LINE__) != VTSS_RC_OK) {
                continue;
            }

            if (port_state == NULL || psec_ifindex->ifindex != psec_ifindex2.ifindex) {
                // We are not manipulating the port state related to the caller,
                // so get and set is OK in this function.
                if (PSEC_interface_status_get(psec_ifindex2.ifindex, &port_state2, __LINE__) != VTSS_RC_OK) {
                    continue;
                }

                new_port_state = &port_state2;
            } else {
                // Do not get and set psec_ifindex->ifindex's port state, because
                // that would overwrite the fields already set.
                new_port_state = port_state;
            }

            new_enable_cpu_copying = FALSE;

            // We should enable secure learning if the switch exists, the port is up,
            // and at least one user-module is enabled.
            new_enable_secure_learning = PSEC_switch_exists[isid_iter] && new_port_state->if_status.status.users != 0 && new_port_state->if_status.status.link_is_up;

            if (new_enable_secure_learning) {
                // We should *disable* CPU copying if
                // 1) Limit is reached and no enabled users want CPU copying to
                //    be kept enabled (PSEC_PORT_MODE_RESTRICT), or
                // 2) Limit is reached and users want CPU copying to remaing enabled, BUT
                //    the current violation count has exceeded it's limit.
                // 3) The port is shut down, or
                // 4) H/W or S/W failures are (still) detected on the port, or
                // 5) At least one user wants the port to be in blocked state
                //    (PSEC_PORT_MODE_KEEP_BLOCKED), where MAC addresses are added
                //    through psec_mgmt_mac_add() and not by any other means, or
                // 6) Learning is not yet enabled, because the PSEC_LIMIT module
                //    hasn't had a chance to add its own static/sticky MAC
                //    addresses.
                //    (PSEC_PORT_MODE_KEEP_BLOCKED), where MAC addresses are added
                //    through psec_mgmt_mac_add() and not by any other means, or
                // 7) There are no Port Security-controlled MAC entries left.
                BOOL disable_due_to_limit_reached = FALSE;

                if (new_port_state->if_status.status.limit_reached) {
                    if (new_port_state->if_status.keep_cpu_copying_enabled) {
                        if (new_port_state->if_status.status.cur_violate_cnt >= new_port_state->if_status.violate_limit) {
                            disable_due_to_limit_reached = TRUE; /* 2 */
                        }
                    } else {
                        disable_due_to_limit_reached = TRUE; /* 1 */
                    }
                }

                new_enable_cpu_copying = !(disable_due_to_limit_reached                                                                      /* 1 + 2 */ ||
                                           new_port_state->notif_status.shut_down                                                            /* 3     */ ||
                                           new_port_state->if_status.status.hw_add_failed || new_port_state->if_status.status.sw_add_failed  /* 4     */ ||
                                           new_port_state->if_status.block_learn_frames                                                      /* 5     */ ||
                                           new_port_state->if_status.block_learn_frames_pending_static_add                                   /* 6     */ ||
                                           PSEC_state.macs_left == 0                                                                         /* 7     */);
            }

            // Now check against the current settings before calling any MAC module API
            // functions.
            old_enable_secure_learning = new_port_state->if_status.status.sec_learning;
            old_enable_cpu_copying     = new_port_state->if_status.status.cpu_copying;

            if ((new_enable_secure_learning != old_enable_secure_learning) ||
                (new_enable_secure_learning && (old_enable_cpu_copying != new_enable_cpu_copying))) {
                PSEC_mac_module_sec_learn_cpu_copy(&psec_ifindex2, new_port_state, new_enable_secure_learning, new_enable_cpu_copying);

                // Save back the new port state unless it's the same port state as being manipulated
                // by the caller of this function.
                if (port_state == NULL || psec_ifindex->ifindex != psec_ifindex2.ifindex) {
                    // It's not the same as the caller. Go change.
                    (void)PSEC_interface_status_set(psec_ifindex2.ifindex, new_port_state, __LINE__);
                }
            }
        }
    }
}

/******************************************************************************/
// PSEC_msg_id_to_str()
/******************************************************************************/
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
static const char *PSEC_msg_id_to_str(psec_msg_id_t msg_id)
{
    switch (msg_id) {
    case PSEC_MSG_ID_PRIM_TO_SEC_RATE_LIMIT_CONF:
        return "PRIM_TO_SEC_RATE_LIMIT_CONF";

    case PSEC_MSG_ID_PRIM_TO_SEC_PORT_CONF:
        return "PRIM_TO_SEC_PORT_CONF";

    case PSEC_MSG_ID_PRIM_TO_SEC_SWITCH_CONF:
        return "PRIM_TO_SEC_SWITCH_CONF";

    case PSEC_MSG_ID_SEC_TO_PRIM_FRAME:
        return "SEC_TO_PRIM_FRAME";

    default:
        return "***Unknown Message ID***";
    }
}
#endif /* VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG */

/****************************************************************************/
// PSEC_user_name()
/****************************************************************************/
static const char *PSEC_user_name(vtss_appl_psec_user_t user)
{
    switch (user) {
    case VTSS_APPL_PSEC_USER_ADMIN:
        return "Port Security (Admin)";

    case VTSS_APPL_PSEC_USER_DOT1X:
        return "802.1X";

    case VTSS_APPL_PSEC_USER_DHCP_SNOOPING:
        return "DHCP Snooping";

    case VTSS_APPL_PSEC_USER_VOICE_VLAN:
        return "Voice VLAN";

    default:
        return "Unknown";
    }
}

/****************************************************************************/
// PSEC_user_abbr()
/****************************************************************************/
static char PSEC_user_abbr(vtss_appl_psec_user_t user)
{
    switch (user) {
    case VTSS_APPL_PSEC_USER_ADMIN:
        return 'P';

    case VTSS_APPL_PSEC_USER_DOT1X:
        return '8';

    case VTSS_APPL_PSEC_USER_DHCP_SNOOPING:
        return 'D';

    case VTSS_APPL_PSEC_USER_VOICE_VLAN:
        return 'V';

    default:
        return 'U'; // Unknown
    }
}

/******************************************************************************/
// PSEC_del_callback()
// PSEC_LIMIT gets called back immediately, whereas other users are called back
// delayed (from the PSEC_thread()) to avoid deadlocks.
/******************************************************************************/
static void PSEC_del_callback(psec_ifindex_t *psec_ifindex, const psec_interface_status_t *const port_state, psec_mac_const_itr_t mac_itr, psec_del_reason_t reason, vtss_appl_psec_user_t user_not_to_be_called_back, vtss_appl_psec_user_t originating_user)
{
    char                   buf[PSEC_MAC_STR_BUF_SIZE];
    vtss_appl_psec_user_t  user;
    psec_mac_add_del_itr_t mac_del_itr;
    bool                   cancel_add = false, erase_it = false, set_del = false;
    u32                    del_users = 0;

    (void)PSEC_mac_itr_to_str(mac_itr, buf);
    T_I("%s: Deleting", buf);

    for (user = (vtss_appl_psec_user_t)0; user < VTSS_APPL_PSEC_USER_LAST; user++) {
        if (user == user_not_to_be_called_back) {
            // Skip calling user
            continue;
        }

        if (PSEC_state.on_mac_del_callbacks[user] == nullptr || !PSEC_USER_ENA_GET(port_state, user)) {
            // User not enabled or doesn't want callbacks
            continue;
        }

        if (user == VTSS_APPL_PSEC_USER_ADMIN) {
            // Synchronous callback for PSEC_LIMIT, since special code is
            // written to avoid deadlocks for that module (which should have
            // been incorporated into this module in the first place).
            PSEC_state.on_mac_del_callbacks[user](psec_ifindex->isid, psec_ifindex->port, &mac_itr->second.status.vid_mac, reason, mac_itr->second.forward_decision[user], originating_user);
        } else {
            // Delayed callback for other users to avoid deadlocks.
            // Get an existing entry or create a new (cleared by constructor).
            mac_del_itr = psec_mac_add_del_map.get(mac_itr->first);
            if (mac_del_itr == psec_mac_add_del_map.end()) {
                T_E("%s: Unable to create MAC del item", buf);
                return;
            }

            // Let's consider the events that can happen without the
            // psec_mac_add_del_map being processed by PSEC_thread():
            //
            // <none>-del:
            //   Simply add a del operation in the delayed callback map.
            //
            // <none>-add:
            //   Simply add an add operation in the delayed callback map.
            //
            // del-del:
            //   It is not possible to get two consecutive del operations,
            //   because the callers of PSEC_del_callback() check that it
            //   already is in the global map (psec_mac_map) before calling
            //   PSEC_del_callback(), after which it is removed from the global
            //   map.
            //
            // add-add:
            //   It is not possible to get two consecutive add operations,
            //   because the caller of PSEC_add_callback() checks that it's not
            //   already in the global map (psec_mac_map) before calling
            //   PSEC_add_callback().
            //
            // del-add:
            //   This is indeed possible, so the delayed callback function must
            //   process dels before adds.
            //
            // add-del:
            //   This is also possible, but when this happens, there's no need
            //   to let users know about it, so the entry gets removed from the
            //   delayed callback map before PSEC_del_callback() returns.
            //
            // del-add-del:
            //   This is also possible. If this happens, the add will be
            //   cancelled and the last del will be ignored.
            if (mac_del_itr->second.members.users_to_call_delayed_add) {
                // Either the add-del or the del-add del situation above.
                if (mac_del_itr->second.members.users_to_call_delayed_del) {
                    // The del-add-del situation.
                    // Cancel the add.
                    cancel_add = true;

                    // And don't add the delete.
                    continue;
                } else {
                    // The add-del situation. Erase this entry.
                    erase_it = true;

                    // And don't add the delete.
                    continue;
                }
            } else if (mac_del_itr->second.members.users_to_call_delayed_del) {
                // The del-del situation.
                T_E("%s: MAC already in progress of being deleted delayed", buf);
                continue;
            } else {
                // The <none>-del situation.
            }

            // Set the user bit in a local variable, so that we don't get the
            // above wrong for other users.
            del_users |= VTSS_BIT(user);
            set_del = true;

            mac_del_itr->second.members.isid                       = psec_ifindex->isid;
            mac_del_itr->second.members.port                       = psec_ifindex->port;
            mac_del_itr->second.members.vid_mac                    = mac_itr->second.status.vid_mac;
            mac_del_itr->second.members.reason                     = reason;
            mac_del_itr->second.members.forward_decision_del[user] = mac_itr->second.forward_decision[user];
            mac_del_itr->second.members.originating_user_del       = originating_user;
        }
    }

    if (erase_it) {
        psec_mac_add_del_map.erase(mac_del_itr);
    } else if (cancel_add) {
        mac_del_itr->second.members.users_to_call_delayed_add = 0;
    } else if (set_del) {
        mac_del_itr->second.members.users_to_call_delayed_del = del_users;

        // Signal to the thread that we want to callback this user.
        vtss_flag_setbits(&PSEC_thread_wait_flag, PSEC_THREAD_WAIT_FLAG_ADD_DEL);
    }
}

/******************************************************************************/
// PSEC_mac_del()
// Removes the MAC address from internal list and MAC table, and calls back
// all that are interested in this - except for user_not_to_be_called_back.
// Finally it checks if this delete gives rise to changing the CPU copy state.
/******************************************************************************/
static void PSEC_mac_del(psec_ifindex_t *psec_ifindex, psec_interface_status_t *port_state, psec_mac_itr_t mac_itr, psec_del_reason_t reason, vtss_appl_psec_user_t user_not_to_be_called_back, vtss_appl_psec_user_t originating_user)
{
    char buf[PSEC_MAC_STR_BUF_SIZE];
    BOOL is_zombie;

    T_D("%s: Deleting, user_not_to_be_called_back = %d", PSEC_mac_itr_to_str(mac_itr, buf), user_not_to_be_called_back);

    PSEC_CRIT_ASSERT_LOCKED();

    // If deleting a zombie (an entry that is not in the MAC table due to S/W or H/W failure),
    // then we don't call back the user modules, since they have no clue that it exists, since
    // they have already been called back when it was determined that there was a S/W or
    // H/W failure. Back then, the entry was just marked as a zombie, but it wasn't really
    // deleted from the list.
    if (mac_itr->second.hw_add_failed || mac_itr->second.sw_add_failed) {
        // We are deleting a zombie. Check to see if this is the last zombie on this port,
        // and if so, change the port state so that we possibly re-enable CPU copying again.
        BOOL zombie_found = FALSE;
        psec_mac_itr_t temp_itr = PSEC_mac_itr_get_first_from_ifindex(mac_itr->first.ifindex);

        while (temp_itr != psec_mac_map.end()) {
            if (temp_itr->first.ifindex > mac_itr->first.ifindex) {
                // Done searching, because the keys are first sorted
                // by ifindex and the iterators ifindex is greater than
                // the ifindex we are looking for.
                break;
            }

            // We don't care about the one we're currently deleting.
            if (mac_itr != temp_itr) {
                if (temp_itr->second.hw_add_failed || temp_itr->second.sw_add_failed) {
                    zombie_found = TRUE; // Still some zombies on this port.
                    break;
                }
            }

            ++temp_itr;
        }

        if (!zombie_found) {
            // We are about to delete the last zombie on this port. Clear the port zombie state flags,
            // so that we possibly re-enable CPU-copying on this port.
            port_state->if_status.status.hw_add_failed = FALSE;
            port_state->if_status.status.sw_add_failed = FALSE;
        }

        is_zombie = TRUE;
    } else {
        // The entry we're deleting is not a zombie.
        // Callback all users enabled on this port and tell them why we delete this entry.
        PSEC_del_callback(psec_ifindex, port_state, mac_itr, reason, user_not_to_be_called_back, originating_user);

        if (mac_itr->second.status.violating) {
            if (port_state->if_status.status.cur_violate_cnt == 0) {
                T_E("%u:%u. Trying to delete a MAC address that indicates violation, but cur_violate_cnt is already 0", psec_ifindex->isid, psec_ifindex->port);
            } else {
                port_state->if_status.status.cur_violate_cnt--;
            }

            mac_itr->second.status.violating = FALSE;
        } else {
            // Deleting an entry that is not in the MAC table because it's violating
            // causes the limit not to be reached anymore.
            // Deleting a zombie, doesn't affect the LIMIT_REACHED state.
            port_state->if_status.status.limit_reached = FALSE;
        }

        is_zombie = FALSE;
    }

    // Now, if the reason this function is called is due to detection of a zombie, we
    // don't actually remove it from the list of MAC addresses on this port, because
    // we need to disable CPU-copying on the port for the duration of the zombie's hold-time.
    // So the PSEC_thread() will still need to count down its hold time and only remove it
    // when it reaches zero.
    if (reason == PSEC_DEL_REASON_HW_ADD_FAILED ||
        reason == PSEC_DEL_REASON_SW_ADD_FAILED) {
        // Special cases where the entry is actually not deleted. Instead a flag is
        // set in both the port's state and the entry that indicates that it couldn't
        // be added to the MAC table. This causes the subsequent call to the function
        // that checks whether to re-enable CPU copying on this port to disable it.
        if (reason == PSEC_DEL_REASON_HW_ADD_FAILED) {
            mac_itr->second.hw_add_failed = TRUE;
            port_state->if_status.status.hw_add_failed = TRUE;
            // First call the function that potentially disables CPU copying on this port.
            PSEC_sec_learn_cpu_copy_check(psec_ifindex, port_state, PSEC_LEARN_CPU_REASON_OTHER, __LINE__);
            // Then delete this entry from the MAC module's MAC table if it's present there
            PSEC_mac_module_del(psec_ifindex, mac_itr);
        } else {
            // Since the MAC module couldn't add this in the first place, it's already not located in the MAC module's
            // MAC table.
            mac_itr->second.sw_add_failed = TRUE;
            port_state->if_status.status.sw_add_failed = TRUE;
            // Since we've just taken a MAC address from the free list, and since it's still taken even
            // though the MAC module returned an error, we must call the cpu-copy check function
            // with MAC_ADDRESS_ALLOCATED, because allocating a MAC address potentially causes
            // all free entries to be taken by now.
            PSEC_sec_learn_cpu_copy_check(psec_ifindex, port_state, PSEC_LEARN_CPU_REASON_MAC_ADDRESS_ALLOCATED, __LINE__);
        }

        // Clear the keep-blocked flag, so that we can age it.
        mac_itr->second.status.kept_blocked = FALSE;

        // Tell the PSEC_thread() to remove this after some time.
        mac_itr->second.status.age_or_hold_time_secs = PSEC_ZOMBIE_HOLD_TIME_SECS;

        // The number of MAC addresses actually in the H/W MAC table is one less now (but this entry stays in
        // our software-list).
        port_state->if_status.status.mac_cnt--;
    } else {
        // First delete this entry from the MAC module's MAC table if it's present there.
        // In the rare case where the PSEC LIMIT module tells us to shut down a port,
        // it's done before the entry is actually added to the MAC module, but all
        // users have been notified that it eventually would (but then again - wouldn't).
        PSEC_mac_module_del(psec_ifindex, mac_itr);

        // Unlink it from this port.
        PSEC_mac_itr_free(mac_itr);

        // Only count down the mac_cnt if this is not a zombie (because if it was a zombie,
        // then it's already not included in the mac_cnt.
        if (!is_zombie) {
            port_state->if_status.status.mac_cnt--;
        }

        // Then check to see if this gave rise to re-enabling CPU-copying on this port.
        PSEC_sec_learn_cpu_copy_check(psec_ifindex, port_state, PSEC_LEARN_CPU_REASON_MAC_ADDRESS_FREED, __LINE__);
    }

    T_D("mac_cnt = %u", port_state->if_status.status.mac_cnt);
}

/******************************************************************************/
// PSEC_mac_del_all()
// Removes all MAC addresses on a specific port from the internal list and MAC
// table.
// All enabled users - except for #user_not_to_be_called_back - will be called
// back with the reason, and the secure learning state and CPU copying state
// will be refreshed.
/******************************************************************************/
static void PSEC_mac_del_all(psec_ifindex_t *psec_ifindex, psec_interface_status_t *port_state, psec_del_reason_t reason, vtss_appl_psec_user_t user_not_to_be_called_back, BOOL also_static)
{
    psec_mac_itr_t mac_itr = PSEC_mac_itr_get_first_from_ifindex(psec_ifindex->ifindex);

    while (mac_itr != psec_mac_map.end()) {
        psec_mac_itr_t mac_itr_next;

        if (mac_itr->first.ifindex > psec_ifindex->ifindex) {
            // Past the right ifindex
            break;
        }

        mac_itr_next = mac_itr;
        mac_itr_next++;

        if (also_static || mac_itr->second.status.mac_type == VTSS_APPL_PSEC_MAC_TYPE_DYNAMIC) {
            PSEC_mac_del(psec_ifindex, port_state, mac_itr, reason, user_not_to_be_called_back, user_not_to_be_called_back);
        }

        mac_itr = mac_itr_next;
    }
}

#if defined(VTSS_SW_OPTION_MSTP)
/******************************************************************************/
// PSEC_mac_del_by_msti()
// Removes all MAC addresses which is matched the specific STP MSTI on a specific
// port from the internal list and MAC table. Since it uses the PSEC_mac_del()
// function, all users that are enabled will be called back with the reason, and
//  the secure learning state and CPU copying state will be refreshed.
/******************************************************************************/
static void PSEC_mac_del_by_msti(psec_ifindex_t *psec_ifindex, psec_interface_status_t *port_state, psec_del_reason_t reason, u8 msti)
{
    char           buf[PSEC_MAC_STR_BUF_SIZE];
    u8             msti_map[VTSS_VIDS];
    psec_mac_itr_t mac_itr;

    if (l2_get_msti_map(msti_map, sizeof(msti_map)) != VTSS_RC_OK) {
        return;
    }

    mac_itr = PSEC_mac_itr_get_first_from_ifindex(psec_ifindex->ifindex);
    while (mac_itr != psec_mac_map.end()) {
        psec_mac_itr_t mac_itr_next;

        if (mac_itr->first.ifindex > psec_ifindex->ifindex) {
            // Done searching, because the keys are first sorted
            // by ifindex and the iterators ifindex is greater than
            // the ifindex we are looking for.
            break;
        }

        mac_itr_next = mac_itr;
        mac_itr_next++;

        if (msti_map[mac_itr->second.status.vid_mac.vid] == msti && mac_itr->second.status.mac_type == VTSS_APPL_PSEC_MAC_TYPE_DYNAMIC) {
            // Only flush dynamic entries, since user has forced sticky and
            // static on this port, whether it is discarding or not.
            T_D("%s: Delete MAC address on MSTI = %d", PSEC_mac_itr_to_str(mac_itr, buf), msti);
            PSEC_mac_del(psec_ifindex, port_state, mac_itr, reason, VTSS_APPL_PSEC_USER_LAST, VTSS_APPL_PSEC_USER_LAST);
        }

        mac_itr = mac_itr_next;
    }
}
#endif /* VTSS_SW_OPTION_MSTP */

/******************************************************************************/
// PSEC_local_port_valid()
/******************************************************************************/
static BOOL PSEC_local_port_valid(mesa_port_no_t port)
{
    if (port >= port_count_max()) {
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************/
//  PSEC_mac_is_unicast()
/******************************************************************************/
static BOOL PSEC_mac_is_unicast(mesa_mac_t *mac)
{
    return (mac->addr[0] & 0x1) ? FALSE : TRUE;
}

/******************************************************************************/
// PSEC_frame_rx()
// Well in fact, it's not only learn frames that we receive here.
// With the software-based aging, we receive all frames from a particular
// source port when the CPU_COPY flag is set in the MAC table.
// We may have a problem with DoS attacks when the CPU_COPY flag is set:
//   Also frames towards the given source port are forwarded to the CPU
//   extraction queues, extracted by the FDMA, and dispatched in the packet
//   module, but since there (probably) ain't any subscribers to these frames,
//   they are discarded. But they end up in the same extraction queue as the
//   management traffic, and may thus cause management traffic to be discarded
//   due to queue overflow. And we won't clear the CPU_COPY flag if a frame
//   destined for the MAC-address is received, since that doesn't guarantee
//   that the specific client is still there.
/******************************************************************************/
static BOOL PSEC_frame_rx(void *contxt, const u8 *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    char                    mac_str[18];
    mesa_port_no_t          port = rx_info->port_no, originating_port = rx_info->port_no;
    mesa_vid_mac_t          vid_mac;
    BOOL                    forward_to_primary_switch = FALSE, frame_consumed = FALSE;
    vtss_isid_t             originating_isid = VTSS_ISID_LOCAL;
    BOOL                    is_primary_switch = msg_switch_is_primary();

    if (port >= port_count_max()) {
        T_D("Received learn frame on invalid source port number (%u). Ignoring it", port);
        return frame_consumed;
    }

    // If it's received on a stack port, we know that we're primary switch here,
    // and we need to dig out the original source port for the frame. This is
    // buried deep down the VStaX header.
    {
        if (is_primary_switch) {
            vtss_isid_t primary_switch_isid;

            if ((primary_switch_isid = msg_primary_switch_isid()) != VTSS_ISID_UNKNOWN) {
                // Translate to a useful ISID rather than VTSS_ISID_LOCAL.
                originating_isid = primary_switch_isid;
            } else {
                // We're the primary switch, but couldn't get our own ISID. In
                // this case, we can't even get the port state, so return from
                // this function.
                return frame_consumed;
            }
        }
    }

    // Gotta reset the vid_mac structure to zeroes in order to be able to memcmp()
    // when comparing to other MACs.
    // The reason is that the vid_mac->mac is 6 bytes, which is padded with 2 bytes
    // to make the next field dword aligned.
    memset(&vid_mac, 0, sizeof(vid_mac));
    vid_mac.vid = rx_info->tag.vid;
    memcpy(vid_mac.mac.addr, &frm[6], sizeof(vid_mac.mac.addr));

    // If it's not a unicast MAC address, also discard it.
    if (!PSEC_mac_is_unicast(&vid_mac.mac)) {
        T_D("Not reacting on multicast source MAC address");
        return frame_consumed;
    }

    {
        PSEC_LOCK_SCOPE();

        if (is_primary_switch) {
            vtss_appl_psec_mac_map_key_t key;
            psec_ifindex_t               psec_ifindex;
            psec_mac_itr_t               mac_itr;
            char                         buf[PSEC_MAC_STR_BUF_SIZE];

            // originating_isid and originating_port tell us where this frame was originally received.
            // See if psec is enabled on that port (which might be a port on a secondary switch), if we
            // received it here on the primary switch's stack port.
            if (PSEC_ifindex_from_port(originating_isid, originating_port, &psec_ifindex, __LINE__) != VTSS_RC_OK) {
                return frame_consumed;
            }

            memset(&key, 0, sizeof(key));
            key.ifindex = psec_ifindex.ifindex;
            key.vlan    = vid_mac.vid;
            key.mac     = vid_mac.mac;

            (void)PSEC_vid_mac_to_str(psec_ifindex.ifindex, &vid_mac, buf);

            mac_itr = PSEC_mac_itr_get(&key);

            // Unfortunately, PSEC is a centralized module, which means that secondary switches don't have insight
            // into MAC addresses and their forwarding decision. Luckily, the frame
            // consumption is only of relevance on the primary switch, because we potentially
            // need only to disallow e.g. IP frames from getting further in the packet
            // rx chain on the primary switch.
            if ((mac_itr = PSEC_mac_itr_get(&key)) != psec_mac_map.end()) {
                frame_consumed = mac_itr->second.status.blocked;
                T_D("%s: Found. Blocked = %d", buf, frame_consumed);
            } else {
                // Port should be kept blocked until potentially opened by PSEC, when
                // this frame has been through the msg_tx() below.
                T_D("%s: NOT found.", buf);
                frame_consumed = TRUE;
            }
        } else {
            // We're secondary switch
            if (VTSS_PORT_BF_GET(PSEC_copy_to_primary_switch, port) == 0) {
                // PSEC is not enabled on the local port.
                return frame_consumed;
            }
        }

        // Check that the frame is not subject to being dropped by the rate-limiter.
        forward_to_primary_switch = !psec_rate_limit_drop(port, &vid_mac);
    }

    if (forward_to_primary_switch) {
        // The rate-limiter tells us to send it to the primary switch.
        // We don't need the mutex anymore, since PSEC_msg_tx_frame() has all the info it needs in the args
        PSEC_msg_tx_frame(originating_isid, originating_port, &vid_mac, 0);
    }

    T_N("<secondary>:%u: Rx Frame (mac=%s, vid=%u, forw. to primary switch=%s, frame consumed=%s)", port, misc_mac_txt(&frm[6], mac_str), rx_info->tag.vid, forward_to_primary_switch ? "yes" : "no", frame_consumed  ? "yes" : "no");
    T_R_HEX(frm, 16);

    return frame_consumed;
}

/******************************************************************************/
// PSEC_learn_frame_rx_register()
/******************************************************************************/
static void PSEC_learn_frame_rx_register(void)
{
    BOOL        chg                    = FALSE;
    BOOL        at_least_one_with_copy = FALSE;
    BOOL        old_copy, new_copy;
    port_iter_t pit;

    (void)port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT);

    while (port_iter_getnext(&pit)) {
        old_copy = VTSS_PORT_BF_GET(PSEC_frame_rx_filter.src_port_mask, pit.iport);
        new_copy = VTSS_PORT_BF_GET(PSEC_copy_to_primary_switch,        pit.iport);

        if (new_copy) {
            at_least_one_with_copy = TRUE;
        }

        if (new_copy != old_copy) {
            chg = TRUE;
            VTSS_PORT_BF_SET(PSEC_frame_rx_filter.src_port_mask, pit.iport, new_copy);
        }
    }

    if (chg) {
        // A change is detected. Gotta register, re-register, or un-register our packet filter.
        if (at_least_one_with_copy) {
            // At least one port is still in copy-to-primary-switch mode.
            if (PSEC_frame_rx_filter_id) {
                // Re-register filter
                (void)packet_rx_filter_change(&PSEC_frame_rx_filter, &PSEC_frame_rx_filter_id);
            } else {
                //  Register new filter
                (void)packet_rx_filter_register(&PSEC_frame_rx_filter, &PSEC_frame_rx_filter_id);
            }
        } else {
            // No more ports in copy-to-primary-switch-mode.
            // Unregister filter.
            if (PSEC_frame_rx_filter_id) {
                (void)packet_rx_filter_unregister(PSEC_frame_rx_filter_id);
                PSEC_frame_rx_filter_id = NULL;
            }
        }
    }
}

/******************************************************************************/
// PSEC_frame_rx_init()
// Receive all kinds of frames. See note in PSEC_frame_rx() header.
/******************************************************************************/
static PSEC_INLINE void PSEC_frame_rx_init(void)
{
    psec_rate_limit_init();
    packet_rx_filter_init(&PSEC_frame_rx_filter);
    PSEC_frame_rx_filter.modid = VTSS_MODULE_ID_PSEC;
    PSEC_frame_rx_filter.match = PACKET_RX_FILTER_MATCH_SRC_PORT;
    PSEC_frame_rx_filter.prio  = PACKET_RX_FILTER_PRIO_BELOW_NORMAL; // We need to be able to stop all but protocol frames
    PSEC_frame_rx_filter.cb    = PSEC_frame_rx;
    // Do not register the filter until we get enabled on at least one port.
}

/******************************************************************************/
// PSEC_mac_address_add_failed()
/******************************************************************************/
static void PSEC_mac_address_add_failed(mesa_vid_mac_t *vid_mac)
{
    char mac_str[18];

    (void)misc_mac_txt(vid_mac->mac.addr, mac_str);

#ifdef VTSS_SW_OPTION_SYSLOG
    S_W("PORT-SECURITY: MAC table full. Could not add <VLAN ID = %u, MAC Addr = %s> to MAC Table", vid_mac->vid, mac_str);
#endif
    T_W("MAC Table Full. Could not add <VLAN ID = %u, MAC Addr = %s> to MAC Table", vid_mac->vid, mac_str);
}

/******************************************************************************/
// PSEC_age_frame_seen()
/******************************************************************************/
static PSEC_INLINE void PSEC_age_frame_seen(psec_ifindex_t *psec_ifindex, psec_mac_itr_t mac_itr)
{
    if (mac_itr->second.status.cpu_copying) {
        mac_itr->second.status.age_frame_seen = TRUE;
        mac_itr->second.status.cpu_copying = FALSE;
        (void)PSEC_mac_module_chg(psec_ifindex, mac_itr, __LINE__);
    }
}

/******************************************************************************/
// PSEC_mac_chg()
// Find the enabled user that contributes with the 'worst' forwarding decision.
// By 'worst' is meant in the order ('worst' in the bottom):
//   Forward
//   Block
//   Keep blocked
// Then update age/hold-times if needed and change or add the MAC address entry
// in the MAC table, if needed.
//
// This function will NOT determine whether CPU copying of learn frames should
// occur or not.
//
// The function can only return FALSE if it calls PSEC_mac_module_chg() and
// if that call fails, which it only can (I think) if we're no longer the
// primary switch, since the MAC address that we change already exists in the
// MAC module's list of MAC addresses.
// In all other cases, it returns TRUE.
/******************************************************************************/
static BOOL PSEC_mac_chg(psec_ifindex_t *psec_ifindex, const psec_interface_status_t *const port_state, psec_mac_itr_t mac_itr, BOOL update_age_hold_times_only)
{
    psec_add_method_t     new_add_method     = PSEC_ADD_METHOD_FORWARD;
    vtss_appl_psec_user_t user;
    u32                   shortest_age_time  = PSEC_AGE_TIME_MAX + 1;
    u32                   shortest_hold_time = PSEC_HOLD_TIME_MAX;
    BOOL                  update_mac_entry   = FALSE;
    BOOL                  result             = TRUE;
    char                  buf[PSEC_MAC_STR_BUF_SIZE];

    if (mac_itr->second.sw_add_failed || mac_itr->second.hw_add_failed) {
        // We cannot change this entry in the MAC table, since it's not
        // there. Also, when one of these flags are set, the forwarding
        // decision set by the user modules is of no good, and the
        // age/hold-time counter is counting the number of seconds to keep
        // the port closed rather than anything user-defined. When
        // that timer expires, this zombie entry will be deleted and the port
        // will be re-enabled.
        return TRUE;
    }

    // All users' forward decision bits are encoded in an u8 array, so we need to use a macro to handle them.
    for (user = (vtss_appl_psec_user_t)0; user < VTSS_APPL_PSEC_USER_LAST; user++) {
        if (PSEC_USER_ENA_GET(port_state, user)) {
            psec_add_method_t add_method = mac_itr->second.forward_decision[user];
            if (add_method > new_add_method) {
                new_add_method = add_method;
            }

            if (PSEC_state.aging_period_secs[user] != 0 && PSEC_state.aging_period_secs[user] < shortest_age_time) {
                // The age period is the shortest non-zero amongst the enabled users.
                // If all have disabled aging (0), then we resort to that.
                shortest_age_time = PSEC_state.aging_period_secs[user];
            }

            if (PSEC_state.hold_time_secs[user] < shortest_hold_time) {
                // The hold times cannot be 0 (disabled)
                shortest_hold_time = PSEC_state.hold_time_secs[user];
            }
        }
    }

    if (mac_itr->second.status.mac_type != VTSS_APPL_PSEC_MAC_TYPE_DYNAMIC ||  shortest_age_time == PSEC_AGE_TIME_MAX + 1) {
        // Either the entry is sticky or static (where aging is disabled) or all
        // users have disabled aging.
        shortest_age_time = 0;
    }

    if (mac_itr->second.status.mac_type != VTSS_APPL_PSEC_MAC_TYPE_DYNAMIC) {
        // Static and sticky entries cannot age out as a result of hold-timeout.
        shortest_hold_time = 0;
    }

    T_N("%s: shortest_age_time = %u, shortest_hold_time = %u, in_mac_module = %d",
        PSEC_mac_itr_to_str(mac_itr, buf),
        shortest_age_time,
        shortest_hold_time,
        mac_itr->second.in_mac_module);

    if (mac_itr->second.in_mac_module) {
        psec_add_method_t cur_add_method;

        // Synthesize the current add method to see if we're gonna
        // update it in the MAC table.
        if (mac_itr->second.status.kept_blocked) {
            cur_add_method = PSEC_ADD_METHOD_KEEP_BLOCKED;
        } else if (mac_itr->second.status.blocked) {
            cur_add_method = PSEC_ADD_METHOD_BLOCK;
        } else {
            cur_add_method = PSEC_ADD_METHOD_FORWARD;
        }

        if (update_age_hold_times_only && cur_add_method != new_add_method) {
            T_E("Internal error");
        }

        switch (cur_add_method) {
        case PSEC_ADD_METHOD_FORWARD:
            switch (new_add_method) {
            case PSEC_ADD_METHOD_FORWARD:
                // Going from forwarding to forwarding.
                // If the previous age period is greater than the new,
                // then use the new (otherwise keep going from where the
                // previous took off).
                // If the new age period is disabled, then make sure
                // CPU copying gets disabled.
                if ((mac_itr->second.status.age_or_hold_time_secs == 0 && shortest_age_time != 0) || mac_itr->second.status.age_or_hold_time_secs > shortest_age_time) {
                    // Adjust the age time if necessary.
                    mac_itr->second.status.age_or_hold_time_secs = shortest_age_time;
                    if (shortest_age_time == 0) {
                        if (mac_itr->second.status.cpu_copying) {
                            mac_itr->second.status.cpu_copying = FALSE;
                            if (update_age_hold_times_only) {
                                // Since we're not going into the update loop below, when
                                // update_age_hold_times_only is set, we need to disable CPU copying
                                // here.
                                (void)PSEC_mac_module_chg(psec_ifindex, mac_itr, __LINE__);
                            } else {
                                update_mac_entry = TRUE;
                            }
                        }
                    }
                }

                break;

            case PSEC_ADD_METHOD_BLOCK:
                // Going from forwarding to blocking. Update the hold time.
                mac_itr->second.status.age_or_hold_time_secs = shortest_hold_time;
            // Fall through

            case PSEC_ADD_METHOD_KEEP_BLOCKED:
            default:
                // Going from forwarding to blocking or keep blocking.
                // Never copy to CPU, but always update the MAC entry.
                mac_itr->second.status.cpu_copying = FALSE;
                update_mac_entry = TRUE;
                break;
            }

            break;

        case PSEC_ADD_METHOD_BLOCK:
            switch (new_add_method) {
            case PSEC_ADD_METHOD_FORWARD:
                // Going from block to forward
                // Always update the MAC entry in the MAC table,
                // but also restart aging and pretend that the frame
                // was received OK in the current age period.
                mac_itr->second.status.cpu_copying = FALSE;
                mac_itr->second.status.age_frame_seen = TRUE;
                mac_itr->second.status.age_or_hold_time_secs = shortest_age_time;
                update_mac_entry = TRUE;
                break;

            case PSEC_ADD_METHOD_BLOCK:
                // Going from block to block. Adjust the remaining hold time
                // if needed.
                if (mac_itr->second.status.age_or_hold_time_secs > shortest_hold_time) {
                    mac_itr->second.status.age_or_hold_time_secs = shortest_hold_time;
                }
                break;

            case PSEC_ADD_METHOD_KEEP_BLOCKED:
                break;

            default:
                // Nothing to do when going from blocked to keep blocked.
                break;
            }

            break;

        case PSEC_ADD_METHOD_KEEP_BLOCKED:
        default:
            switch (new_add_method) {
            case PSEC_ADD_METHOD_FORWARD:
                // Going from keep-blocked to forward.
                // Always update the MAC entry in the MAC table,
                // but also restart aging and pretend that the frame
                // was received OK in the current age period.
                mac_itr->second.status.cpu_copying = FALSE;
                mac_itr->second.status.age_frame_seen = TRUE;
                mac_itr->second.status.age_or_hold_time_secs = shortest_age_time;
                update_mac_entry = TRUE;
                break;

            case PSEC_ADD_METHOD_BLOCK:
                // Going from keep-blocked to blocked.
                // Start the hold timer.
                mac_itr->second.status.age_or_hold_time_secs = shortest_hold_time;
                break;

            case PSEC_ADD_METHOD_KEEP_BLOCKED:
            default:
                // Nothing to do
                break;
            }

            break;
        } /* switch (cur_add_method) */
    } else {
        // The MAC address is currently not in the table (it's brandnew)
        update_mac_entry = TRUE;
        mac_itr->second.status.cpu_copying = FALSE; // Superfluous.

        switch (new_add_method) {
        case PSEC_ADD_METHOD_FORWARD:
            // Pretend an age frame is seen in the first period.
            mac_itr->second.status.age_frame_seen = TRUE;
            mac_itr->second.status.age_or_hold_time_secs = shortest_age_time;
            break;

        case PSEC_ADD_METHOD_BLOCK:
            mac_itr->second.status.age_or_hold_time_secs = shortest_hold_time;
            break;

        case PSEC_ADD_METHOD_KEEP_BLOCKED:
        default:
            break;
        }
    }

    if (!update_age_hold_times_only) {
        switch (new_add_method) {
        case PSEC_ADD_METHOD_FORWARD:
            mac_itr->second.status.blocked = FALSE;
            mac_itr->second.status.kept_blocked = FALSE;
            break;

        case PSEC_ADD_METHOD_BLOCK:
            // BLOCK is used to block the entry for a while (subject to 'aging').
            mac_itr->second.status.blocked = TRUE;
            mac_itr->second.status.kept_blocked = FALSE;
            break;

        case PSEC_ADD_METHOD_KEEP_BLOCKED:
            // KEEP_BLOCKED will keep it in the table indefinitely (not subject to 'aging').
            mac_itr->second.status.blocked = TRUE;
            mac_itr->second.status.kept_blocked = TRUE;
            break;

        default:
            T_E("Invalid decision");
            return FALSE;
        }

        if (update_mac_entry) {
            result = PSEC_mac_module_chg(psec_ifindex, mac_itr, __LINE__);
        }
    }

    return result;
}

/******************************************************************************/
// PSEC_add_callback()
/******************************************************************************/
static void PSEC_add_callback(psec_ifindex_t            *psec_ifindex,
                              vtss_appl_psec_mac_conf_t *mac_conf,
                              psec_interface_status_t   *port_state,
                              psec_mac_itr_t            mac_itr,
                              psec_add_method_t         add_method_by_calling_user,
                              psec_add_action_t         *worst_case_add_action,
                              vtss_appl_psec_user_t     user_not_to_be_called_back,
                              BOOL                      delayed_callback)
{
    char                   buf[PSEC_MAC_STR_BUF_SIZE];
    psec_add_method_t      user_add_method;
    vtss_appl_psec_user_t  user;
    mesa_vid_mac_t         vid_mac;
    bool                   set_add = false;
    u32                    add_users = 0;
    psec_mac_add_del_itr_t mac_add_itr;

    PSEC_CRIT_ASSERT_LOCKED();

    (void)PSEC_mac_itr_to_str(mac_itr, buf);

    T_I("%s: Adding", buf);

    *worst_case_add_action = PSEC_ADD_ACTION_NONE;
    PSEC_mac_conf_to_vid_mac(mac_conf, &vid_mac);

    for (user = (vtss_appl_psec_user_t)0; user < VTSS_APPL_PSEC_USER_LAST; user++) {
        if (PSEC_state.on_mac_add_callbacks[user] == nullptr || !PSEC_USER_ENA_GET(port_state, user)) {
            // User is not enabled or hasn't installed a callback.
            continue;
        }

        if (user == user_not_to_be_called_back) {
            // This is added by the originating user. We know her add method
            // already and she knows she is not going to be called back.
            user_add_method = add_method_by_calling_user;
        } else if (user == VTSS_APPL_PSEC_USER_ADMIN) {
            BOOL set_violate_mac = FALSE;

            // Synchronous callback for PSEC_LIMIT, since special code is
            // written to avoid deadlocks for that module (which should have
            // been incorporated into this module in the first place).
            user_add_method = PSEC_state.on_mac_add_callbacks[user](psec_ifindex->isid, psec_ifindex->port, &vid_mac, port_state->if_status.status.mac_cnt - 1, delayed_callback ? VTSS_APPL_PSEC_USER_ADMIN : user_not_to_be_called_back, worst_case_add_action);

            // If ADMIN user's port mode is PSEC_PORT_MODE_RESTRICT, we have
            // promised her to count whenever she returns a blocked MAC address.
            if (port_state->if_status.port_mode[user] == PSEC_PORT_MODE_RESTRICT && user_add_method == PSEC_ADD_METHOD_BLOCK) {
                if (*worst_case_add_action == PSEC_ADD_ACTION_LIMIT_REACHED) {
                    port_state->notif_status.total_violate_cnt++;
                    port_state->if_status.status.cur_violate_cnt++;
                    mac_itr->second.status.violating = TRUE;
                    set_violate_mac = TRUE;
                } else if (*worst_case_add_action == PSEC_ADD_ACTION_SHUT_DOWN) {
                    set_violate_mac = TRUE;
                }

                if (set_violate_mac) {
                    port_state->notif_status.latest_violating_vlan = vid_mac.vid;
                    port_state->notif_status.latest_violating_mac  = vid_mac.mac;
                    mac_itr->second.status.mac_type = VTSS_APPL_PSEC_MAC_TYPE_DYNAMIC;
                }
            }
        } else {
            // Delayed callback for other users to avoid deadlocks.
            // Get an existing entry or create a new (cleared by constructor).
            mac_add_itr = psec_mac_add_del_map.get(mac_itr->first);
            if (mac_add_itr == psec_mac_add_del_map.end()) {
                T_E("%s: Unable to create MAC add item", buf);
                return;
            }

            // See possible situations in PSEC_del_callback().
            if (mac_add_itr->second.members.users_to_call_delayed_add) {
                // The add-add situation.
                T_E("%s: MAC already in progress of being added delayed", buf);
                continue;
            } else {
                // The <none>-add or del-add situation. Either way, we need to
                // add it.
                set_add = true;

                // Set the user bit in a local variable, so that we don't get
                // the above wrong for other users.
                add_users |= VTSS_BIT(user);
            }

            mac_add_itr->second.members.isid                 = psec_ifindex->isid;
            mac_add_itr->second.members.port                 = psec_ifindex->port;
            mac_add_itr->second.members.vid_mac              = vid_mac;
            mac_add_itr->second.members.originating_user_add = user_not_to_be_called_back;
            mac_add_itr->second.members.unique               = mac_itr->second.unique;

            // Keep this MAC address blocked until this user has actually been
            // called back (from PSEC_thread()).
            user_add_method = PSEC_ADD_METHOD_KEEP_BLOCKED;
        }

        mac_itr->second.forward_decision[user] = user_add_method;
    }

    if (set_add) {
        mac_add_itr->second.members.users_to_call_delayed_add = add_users;

        // Signal to the thread that we want to callback some users delayed.
        vtss_flag_setbits(&PSEC_thread_wait_flag, PSEC_THREAD_WAIT_FLAG_ADD_DEL);
    }
}

/******************************************************************************/
// PSEC_on_shutdown_recover()
/******************************************************************************/
static void PSEC_on_shutdown_recover(psec_ifindex_t *psec_ifindex)
{
    psec_interface_status_t port_state;

    {
        PSEC_LOCK_SCOPE();

        if (PSEC_interface_status_get(psec_ifindex->ifindex, &port_state, __LINE__) != VTSS_RC_OK) {
            return;
        }

        // From a port security point of view, the port is no longer shut down.
        port_state.notif_status.shut_down = FALSE;

        // This may have given rise to enabling secure learning on that port.
        PSEC_sec_learn_cpu_copy_check(psec_ifindex, &port_state, PSEC_LEARN_CPU_REASON_OTHER, __LINE__);

        // Save any changes back to the state.
        (void)PSEC_interface_status_set(psec_ifindex->ifindex, &port_state, __LINE__);
    }
}


/******************************************************************************/
// PSEC_on_port_adm_shutdown_callback()
// Invoked when end-user has issued a "shutdown" on a given port.
/******************************************************************************/
static void PSEC_on_port_adm_shutdown_callback(mesa_port_no_t port, const port_vol_conf_t *const port_vol_conf)
{
    psec_ifindex_t psec_ifindex;

    if (PSEC_ifindex_from_port(VTSS_ISID_START, port, &psec_ifindex, __LINE__) != VTSS_RC_OK) {
        return;
    }

    PSEC_on_shutdown_recover(&psec_ifindex);
}


































/******************************************************************************/
// PSEC_port_vol_conf_set()
/******************************************************************************/
static void PSEC_port_vol_conf_set(psec_ifindex_t *psec_ifindex, BOOL shut_down)
{
    port_vol_conf_t port_vol_conf;

    if (port_vol_conf_get(PORT_USER_PSEC, psec_ifindex->port, &port_vol_conf) != VTSS_RC_OK) {
        T_E("Unable to obtain port volatile configuration for %u", psec_ifindex->port);
        // Go on anyway. What else to do?
        memset(&port_vol_conf, 0, sizeof(port_vol_conf));
    }

    T_I("%u port status before = %d, after = %d", psec_ifindex->port, port_vol_conf.disable_adm_recover, shut_down);

    port_vol_conf.disable_adm_recover  = shut_down;
    port_vol_conf.on_adm_recover_clear = PSEC_on_port_adm_shutdown_callback;

    if (port_vol_conf_set(PORT_USER_PSEC, psec_ifindex->port, &port_vol_conf) != VTSS_RC_OK) {
        T_E("Unable to set port volatile configuration for %u", psec_ifindex->port);
        // Go on anyway. What else to do?
    }
}



























/******************************************************************************/
// PSEC_shutdown_set()
// After having called this function with shut_down == TRUE, the only way
// to get the port up again is to issue a shutdown/no shutdown on the port
// or make administrative port-security configuration changes on the port or
// - if errdisable auto-recovery is enabled - wait for the port to time out.
/******************************************************************************/
static void PSEC_shutdown_set(psec_ifindex_t *psec_ifindex, BOOL shut_down)
{



    PSEC_port_vol_conf_set(psec_ifindex, shut_down);

}

/******************************************************************************/
// PSEC_mac_add()
/******************************************************************************/
static mesa_rc PSEC_mac_add(psec_ifindex_t *psec_ifindex, psec_interface_status_t *port_state, vtss_appl_psec_mac_conf_t *mac_conf, psec_add_method_t add_method_by_calling_user, vtss_appl_psec_user_t user_not_to_be_called_back, BOOL delayed_callback)
{
    psec_mac_itr_t               mac_itr;
    vtss_appl_psec_mac_map_key_t key;
    psec_add_action_t            worst_case_add_action;
    char                         buf[PSEC_MAC_STR_BUF_SIZE];

    (void)PSEC_mac_conf_to_str(psec_ifindex->ifindex, mac_conf, buf);
    T_D("%s. User = %d", buf, user_not_to_be_called_back);

    // Only add it if the switch actually exists. The reason why we can end here is
    // that the entry may have been stored in the H/W Rx Queue before we were
    // notified that the switch was deleted.
    if (!PSEC_switch_exists[psec_ifindex->isid - VTSS_ISID_START]) {
        T_D("%s. Switch is down", buf);
        return VTSS_APPL_PSEC_RC_SWITCH_IS_DOWN;
    }

#if defined(VTSS_SW_OPTION_MSTP)
    if (port_state->if_status.status.stp_discarding && !delayed_callback) {
        // Do more check here since there is at least one STP MSTI in discarding
        // state on the port. Deny this process when the port's STP MSTI state
        // is in discarding state on specific VLAN - unless the PSEC_LIMIT
        // module is forcing us to add this MAC (delayed_callback == TRUE).
        u8 msti_map[VTSS_VIDS];

        if (l2_get_msti_map(msti_map, sizeof(msti_map)) == VTSS_RC_OK) {
            u8 msti;

            for (msti = 0; msti < N_MSTI_MAX; msti++) {
                if (l2_get_msti_stpstate(msti, L2PORT2PORT(psec_ifindex->isid, psec_ifindex->port)) == VTSS_COMMON_STPSTATE_DISCARDING && msti_map[mac_conf->vlan] == msti) {
                    // VID present in mask
                    char buf[PSEC_MAC_STR_BUF_SIZE];
                    T_D("%s: Deny new dynamic MAC entry", buf);
                    return VTSS_APPL_PSEC_RC_STP_MSTI_DISCARDING;
                }
            }
        }
    }
#endif /* VTSS_SW_OPTION_MSTP */

    // The port must have link and the limit may not have been reached and the port
    // must not have been shut-down by the PSEC LIMIT module, and at least one user must
    // be enabled on this port.
    // It is perfectly normal to get here even if CPU copying is disabled.
    // One reason is that broadcast frames are sent to the CPU due to a statically entered
    // MAC address in the MAC table, and so are L2 protocol frames, which may or may not
    // be forwarded all the way to the PSEC module. Also, any user module may call
    // psec_mgmt_mac_add() which will end up in this code as well.
    if (!delayed_callback && !port_state->if_status.status.link_is_up) {
        T_D("%s. Link is down", buf);
        return VTSS_APPL_PSEC_RC_LINK_IS_DOWN;
    }

    if (port_state->if_status.status.limit_reached) {
        if (port_state->if_status.keep_cpu_copying_enabled) {
            // Could be we shouldn't ask anyone about this MAC anyway, because
            // the maximum violation count could be exceeded as well.
            if (port_state->if_status.status.cur_violate_cnt >= port_state->if_status.violate_limit) {
                T_D("%s. Limit is reached", buf);
                return VTSS_APPL_PSEC_RC_LIMIT_IS_REACHED;
            }
        } else {
            // Limit is reached and we're not asked to keep the port open.
            T_D("%s. Limit is reached", buf);
            return VTSS_APPL_PSEC_RC_LIMIT_IS_REACHED;
        }
    }

    if (port_state->notif_status.shut_down && !delayed_callback) {
        T_D("%s. Port is shut down", buf);
        return VTSS_APPL_PSEC_RC_PORT_IS_SHUT_DOWN;
    }

    if (port_state->if_status.status.users == 0) {
        T_D("%s. Port has no enabled users", buf);
        return VTSS_APPL_PSEC_RC_NO_USERS_ENABLED;
    }

    // Ask all enabled modules on this port how we should treat this new entry.

    // Allocate a new entry and attach it to the port. The function will clear
    // all fields and set those that must be set.
    memset(&key, 0, sizeof(key));
    key.ifindex = psec_ifindex->ifindex;
    key.vlan    = mac_conf->vlan;
    key.mac     = mac_conf->mac;

    if ((mac_itr = PSEC_mac_itr_alloc(&key)) == psec_mac_map.end()) {
        return VTSS_APPL_PSEC_RC_MAC_POOL_DEPLETED;
    }

    mac_itr->second.status.mac_type = mac_conf->mac_type;
    port_state->if_status.status.mac_cnt++;

    // Callback PSEC LIMIT synchronously (deadlock avoidance is taken care of
    // already) and other users asynchronously (to avoid deadlocks).
    PSEC_add_callback(psec_ifindex, mac_conf, port_state, mac_itr, add_method_by_calling_user, &worst_case_add_action, user_not_to_be_called_back, delayed_callback);

    switch (worst_case_add_action) {
    case PSEC_ADD_ACTION_NONE:
        // Hey, the PSEC LIMIT is OK with this entry.
        port_state->if_status.status.limit_reached = FALSE;
        break;

    case PSEC_ADD_ACTION_LIMIT_REACHED:
        // Well, add this entry, but possibly stop CPU copying.
        T_D("Limit Reached");
        port_state->if_status.status.limit_reached = TRUE;
        break;

    case PSEC_ADD_ACTION_SHUT_DOWN:
        T_D("Port Shut Down");
        // Ouch. This one caused an overflow of allowed MAC addresses.
        // We have to delete all entries on this port. The port will
        // not be usable again until end-user has administratively shutdown/no shutdown the port.
        port_state->notif_status.shut_down = TRUE;
        port_state->if_status.status.limit_reached = FALSE;

        PSEC_shutdown_set(psec_ifindex, TRUE);

        // The following call will also cause the CPU copying state to be updated.
        // We need to mask out the calling user before calling this function, because
        // otherwise he would get called back, and the calling user would rather
        // get it in the return code to psec_mgmt_mac_add().
        // And we're already damn sure that the user is actually enabled on this port.
        PSEC_mac_del_all(psec_ifindex, port_state, PSEC_DEL_REASON_PORT_SHUT_DOWN, user_not_to_be_called_back, FALSE);

        // Nothing more to do here.
        T_D("%s. Now, the port is shut down", buf);
        return VTSS_APPL_PSEC_RC_PORT_IS_SHUT_DOWN;

    default:
        T_E("User-module returned invalid action");
        return VTSS_APPL_PSEC_RC_INTERNAL_ERROR;
    }

    // If we get here, we should try to add the MAC address to the MAC module.
    // This function will also determine how the MAC address should be added,
    // i.e. blocked or forwarding, based on the user-decisions, and it will
    // determine the aging/hold.
    if (PSEC_mac_chg(psec_ifindex, port_state, mac_itr, FALSE)) {
        // Update the cpu-copy enable/disable state
        PSEC_sec_learn_cpu_copy_check(psec_ifindex, port_state, PSEC_LEARN_CPU_REASON_MAC_ADDRESS_ALLOCATED, __LINE__);
    } else {
        // The add failed due to the MAC module (S/W fail).
        // The call of the following function will also update the cpu-copying enable/disable.
        T_D("%s. Unable to change CPU copy state. Deleting it again", buf);
        PSEC_mac_del(psec_ifindex, port_state, mac_itr, PSEC_DEL_REASON_SW_ADD_FAILED, user_not_to_be_called_back, delayed_callback ? VTSS_APPL_PSEC_USER_ADMIN : user_not_to_be_called_back);
    }

    T_D("%s. Success", buf);
    return VTSS_RC_OK;
}

/******************************************************************************/
// PSEC_handle_frame_reception()
/******************************************************************************/
static PSEC_INLINE void PSEC_handle_frame_reception(psec_ifindex_t *psec_ifindex, psec_msg_frame_t *msg)
{
    psec_interface_status_t port_state;
    psec_mac_itr_t          mac_itr;
    mesa_vid_t              fid = psec_limit_mgmt_fid_get(msg->vid_mac.vid);
    BOOL                    add_new = TRUE;
    char                    buf[PSEC_MAC_STR_BUF_SIZE];

    (void)PSEC_vid_mac_to_str(psec_ifindex->ifindex, &msg->vid_mac, buf);

    // Received a frame, which is now forwarded to the primary switch.
    // Check to see if we have that MAC address in our list already.
    mac_itr = PSEC_mac_itr_get_from_fid_mac(fid, &msg->vid_mac.mac);

    if (mac_itr != psec_mac_map.end()) {
        // Already assumed added to the MAC table.
        if (mac_itr->first.vlan != msg->vid_mac.vid) {
            // I think we must treat two frames with same MAC and different VIDs
            // that map to the same FID as being identical to two frames with
            // same MAC and same VID, so no code in this if().
        }

        if (mac_itr->first.ifindex != psec_ifindex->ifindex) {
            // Received it on another port.
            psec_ifindex_t looked_up_psec_ifindex;

            // If the entry is static/sticky, don't move it.
            if (mac_itr->second.status.mac_type != VTSS_APPL_PSEC_MAC_TYPE_DYNAMIC) {
                T_D("%s: Already learned as %s on ifindex = %u. Skipping", buf, psec_util_mac_type_to_str(mac_itr->second.status.mac_type), VTSS_IFINDEX_PRINTF_ARG(mac_itr->first.ifindex));
                return;
            }

            if (PSEC_ifindex_from_ifindex(mac_itr->first.ifindex, &looked_up_psec_ifindex, __LINE__) != VTSS_RC_OK) {
                return;
            }

            if ((PSEC_interface_status_get(looked_up_psec_ifindex.ifindex, &port_state, __LINE__)) != VTSS_RC_OK) {
                return;
            }

            T_D("%s: Already learned as %s on ifindex = %u. Deleting current", buf, psec_util_mac_type_to_str(mac_itr->second.status.mac_type), VTSS_IFINDEX_PRINTF_ARG(mac_itr->first.ifindex));
            PSEC_mac_del(&looked_up_psec_ifindex, &port_state, mac_itr, PSEC_DEL_REASON_STATION_MOVED, VTSS_APPL_PSEC_USER_LAST, VTSS_APPL_PSEC_USER_LAST);

            if (PSEC_interface_status_set(looked_up_psec_ifindex.ifindex, &port_state, __LINE__) != VTSS_RC_OK) {
                return;
            }
        } else if (msg->is_learn_frame) {
            // Already known on this interface.
            T_D("%s: Already learned on this interface, but it's marked as a learn frame", buf);

            // If it's a learn frame, then the reason for getting here is twofold:
            // 1) The client got to send two or more frames before we reacted on the first
            // 2) There wasn't room in the MAC table
            add_new = FALSE; // Do not give rise to adding it again.
            if (mac_itr->second.hw_add_failed || mac_itr->second.sw_add_failed) {
                // Silently discard this, because we're already aware that this is
                // a MAC address that cannot be added to the MAC table (hash overflow or MAC module failure).
            } else if (msg_uptime_get(VTSS_ISID_LOCAL) - mac_itr->second.creation_time_secs >= 7) {
                // It's more than 7 seconds ago we added this MAC address to the table.
                // For testing, use these 5 MAC addresses, which all map to row 21 of the MAC table.
                // 00-00-00-00-00-05 1
                // 00-00-00-00-08-04 1
                // 00-00-00-00-10-07 1
                // 00-00-00-00-18-06 1
                // 00-00-00-00-20-01 1
                // Only report this once per MAC address per hold time.
                PSEC_mac_address_add_failed(&mac_itr->second.status.vid_mac);
                PSEC_mac_del(psec_ifindex, &port_state, mac_itr, PSEC_DEL_REASON_HW_ADD_FAILED, VTSS_APPL_PSEC_USER_LAST, VTSS_APPL_PSEC_USER_LAST);
            }
        } else {
            T_D("%s: Age frame", buf);

            // It's not a learn frame, but we do have information stored about this MAC address.
            // This means that we're probably aging the entry. Stop the CPU copying.
            PSEC_age_frame_seen(psec_ifindex, mac_itr);
            add_new = FALSE;
        }
    } else {
        T_D("%s: Not known. Will learn", buf);
    }

    if (add_new) {
        if ((PSEC_interface_status_get(psec_ifindex->ifindex, &port_state, __LINE__)) != VTSS_RC_OK) {
            return;
        }

        // Gotta check if not at least one user module wants the port to be blocking.
        // If so, we silently discard this frame, because all MAC addresses in that
        // case must be learned through the psec_mgmt_mac_add() function call,
        // which in turn then calls all user modules and asks for their opinion.
        if (!port_state.if_status.block_learn_frames && !port_state.if_status.block_learn_frames_pending_static_add) {
            vtss_appl_psec_mac_conf_t mac_conf;

            // No-one thinks the port should be blocking.
            // Add it if there are state machines left. If there aren't any state machines left,
            // then this MAC address is one that were stored in the H/W Rx Queue before CPU copying
            // got to be turned off.

            PSEC_vid_mac_to_mac_conf(&msg->vid_mac, &mac_conf, port_state.if_status.status.sticky ? VTSS_APPL_PSEC_MAC_TYPE_STICKY : VTSS_APPL_PSEC_MAC_TYPE_DYNAMIC);
            T_D("%s: Adding new", buf);
            (void)PSEC_mac_add(psec_ifindex, &port_state, &mac_conf, PSEC_ADD_METHOD_FORWARD /* Dummy method */, VTSS_APPL_PSEC_USER_LAST /* Indicates that all enabled users should be asked */, FALSE);

            // Always save the new state back.
            (void)PSEC_interface_status_set(psec_ifindex->ifindex, &port_state, __LINE__);
        } else {
            T_D("%s: Skipping because port is blocked for learn-frames", buf);
        }
    }
}

/******************************************************************************/
// PSEC_msg_rx()
/******************************************************************************/
static BOOL PSEC_msg_rx(void *contxt, const void *const the_rxd_msg, size_t len, vtss_module_id_t modid, u32 isid)
{
    psec_msg_t     *rx_msg = (psec_msg_t *)the_rxd_msg;
    psec_ifindex_t psec_ifindex;
    char           buf[PSEC_MAC_STR_BUF_SIZE];

    T_N("msg_id: %d, %s, ver: %u, len: %zd, isid: %u", rx_msg->hdr.msg_id, PSEC_msg_id_to_str(rx_msg->hdr.msg_id), rx_msg->hdr.version, len, isid);

    // Check if we support this version of the message. If not, print a warning and return.
    if (rx_msg->hdr.version != PSEC_MSG_VERSION) {
        T_W("Unsupported version of the message (%u)", rx_msg->hdr.version);
        return TRUE;
    }

    switch (rx_msg->hdr.msg_id) {
    case PSEC_MSG_ID_PRIM_TO_SEC_RATE_LIMIT_CONF: {
        psec_msg_rate_limit_conf_t *msg = &rx_msg->u.rate_limit_conf;
        psec_rate_limit_conf_set(VTSS_ISID_LOCAL, &msg->rate_limit);
        break;
    }

    case PSEC_MSG_ID_PRIM_TO_SEC_PORT_CONF: {
        // Set what and when to copy to primary switch.
        psec_msg_port_conf_t *msg = &rx_msg->u.port_conf;
        if (!PSEC_local_port_valid(msg->port)) {
            break;
        }

        // Since we call the PSEC_learn_frame_rx_register(), which in turn waits
        // for the packet_cfg crit, we may end up in a deadlock if we used
        // PSEC_CRIT_ENTER() here, because it may be that the following sequence of events
        // takes place:
        //  1) Right here we take and get psec_crit.
        //  2) A learn frame arrives in the packet module, and packet_cfg crit gets taken in RX_dispatch()
        //  3) RX_dispatch() calls back PSEC module (PSEC_frame_rx()), which starts by taking the psec_crit, that is, it waits for this piece of code to finish.
        //  4) We call PSEC_learn_frame_rx_register(), which waits for packet_cfg crit, which is already taken by RX_dispatch().
        //  5) Deadlock!
        // Solution: Let's assume that we take psec_crit and update PSEC_copy_to_primary_switch[] array, release psec_crit and call PSEC_learn_frame_rx_register().
        // There are two things to consider here:
        //   a) Protection of PSEC_frame_rx_filter_id. This is inherintly protected since the PSEC_learn_frame_rx_register() can only be called
        //      from within the PSEC_msg_rx() function, which is in the Msg Rx thread context.
        //   b) Protection of the PSEC_copy_to_primary_switch[] array. Only one thread can write that array (after boot), and that's the this function (PSEC_msg_rx()).
        //      Whether PSEC_frame_rx() reads a 0 or a 1 from PSEC_copy_to_primary_switch[] when it receives a frame is not critical. It's critical, however, that
        //      PSEC_learn_frame_rx_register() actually detects changes and calls the packet_rx_filter_register()/unregister() function appropriately, but
        //      - again - since that function is only called from this function (PSEC_msg_rx()), and since this function can only be called from the Msg Rx
        //      thread, we should be safe.
        PSEC_CRIT_ENTER();
        VTSS_PORT_BF_SET(PSEC_copy_to_primary_switch, msg->port, msg->copy_to_primary_switch);
        // Ask the rate-limiter to clear its own filter for this port.
        psec_rate_limit_filter_clr(msg->port);
        PSEC_CRIT_EXIT();
        PSEC_learn_frame_rx_register(); // Figures out changes itself.
        break;
    }

    case PSEC_MSG_ID_PRIM_TO_SEC_SWITCH_CONF: {
        // Set what and when to copy to primary switch.
        psec_msg_switch_conf_t *msg = &rx_msg->u.switch_conf;

        PSEC_CRIT_ENTER();
        memcpy(PSEC_copy_to_primary_switch, msg->copy_to_primary_switch, sizeof(PSEC_copy_to_primary_switch));
        // Ask the rate-limiter to clear its own filter for the whole switch.
        psec_rate_limit_filter_clr(fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT));
        PSEC_CRIT_EXIT();

        PSEC_learn_frame_rx_register(); // Figures out changes itself.
        break;
    }

    case PSEC_MSG_ID_SEC_TO_PRIM_FRAME: {
        if (!msg_switch_is_primary()) {
            return TRUE;
        }

        if (PSEC_ifindex_from_port(rx_msg->u.frame.isid == VTSS_ISID_LOCAL ? isid : rx_msg->u.frame.isid, rx_msg->u.frame.port, &psec_ifindex, __LINE__) != VTSS_RC_OK) {
            return TRUE;
        }

        T_N("%s: Frame Rx, learn = %d", PSEC_vid_mac_to_str(psec_ifindex.ifindex, &rx_msg->u.frame.vid_mac, buf), rx_msg->u.frame.is_learn_frame);

        PSEC_CRIT_ENTER();
        PSEC_handle_frame_reception(&psec_ifindex, &rx_msg->u.frame);
        PSEC_CRIT_EXIT();
        break;
    }

    default:
        T_D("Unknown message ID: %d", rx_msg->hdr.msg_id);
        break;
    }

    return TRUE;
}

/******************************************************************************/
// PSEC_msg_rx_init()
/******************************************************************************/
static void PSEC_msg_rx_init(void)
{
    msg_rx_filter_t filter;

    memset(&filter, 0, sizeof(filter));
    filter.cb = PSEC_msg_rx;
    filter.modid = VTSS_MODULE_ID_PSEC;
    (void)msg_rx_filter_register(&filter);
}

/******************************************************************************/
// PSEC_static_macs_handle()
/******************************************************************************/
static void PSEC_static_macs_handle(psec_ifindex_t *psec_ifindex, psec_interface_status_t *port_state, BOOL force)
{
    PSEC_CRIT_ASSERT_LOCKED();

    if (port_state->if_status.adding_static_macs) {
        // The other function calling this function is already handling this.
        T_D("Already in this function");
        return;
    }

    // While adding static/sticky MAC entries, the PSEC mutex gets released and
    // re-taken per MAC address. This means that we can risk removing all MAC
    // entries in one thread while adding static/sticky in another. This state
    // variable prevents this from happening.
    port_state->if_status.adding_static_macs = TRUE;

    // While possibly adding sticky/static MAC addresses, prevent learn frames
    // from being added.
    port_state->if_status.block_learn_frames_pending_static_add = TRUE;

    if (PSEC_USER_ENA_GET(port_state, VTSS_APPL_PSEC_USER_ADMIN)) {
        // Time to add static MACs. First save the port state, because it may
        // change while we're inside the psec_limit_mgmt_macs_get() function,
        // because the PSEC mutex may get released and re-taken per MAC address
        // that gets added. After we've added what we need, reload the port
        // state.
        // We do this whether or not the port is up.
        T_D("Invoking psec_limit_mgmt_static_macs_get(force = %d)", force);
        (void)PSEC_interface_status_set(psec_ifindex->ifindex, port_state, __LINE__);
        psec_limit_mgmt_static_macs_get(psec_ifindex->ifindex, force);
        (void)PSEC_interface_status_get(psec_ifindex->ifindex, port_state, __LINE__);
        T_D("Done invoking psec_limit_mgmt_static_macs_get(force = %d)", force);
    }

    // If in the meanwhile the link went down, we gotta remove all non-static-MACs again.
    // The PSEC_mac_del_all() is not releasing the PSEC mutex, so here we are
    // safe.
    if (!port_state->if_status.status.link_is_up) {
        // Don't remove static/sticky MAC addresses even when the port is down.
        T_D("Removing all MAC addresses on interface = %u", VTSS_IFINDEX_PRINTF_ARG(psec_ifindex->ifindex));
        PSEC_mac_del_all(psec_ifindex, port_state, PSEC_DEL_REASON_PORT_LINK_DOWN, VTSS_APPL_PSEC_USER_LAST, FALSE);
    }

    // After we've now possible added static/sticky MAC addresses, we can open
    // for learn frames again.
    port_state->if_status.block_learn_frames_pending_static_add = FALSE;

    // Check whether we need to get frames to the CPU again.
    PSEC_sec_learn_cpu_copy_check(psec_ifindex, port_state, PSEC_LEARN_CPU_REASON_OTHER, __LINE__);

    // Done with this exercise.
    port_state->if_status.adding_static_macs = FALSE;

    // Save the port state back
    (void)PSEC_interface_status_set(psec_ifindex->ifindex, port_state, __LINE__);
}

/******************************************************************************/
// PSEC_link_state_change_callback()
/******************************************************************************/
static void PSEC_link_state_change_callback(mesa_port_no_t port, const vtss_appl_port_status_t *status)
{
    psec_ifindex_t          psec_ifindex;
    psec_interface_status_t port_state;

    T_I("Link state change on port %u. New link state = %d", port, status->link);

    if (PSEC_ifindex_from_port(VTSS_ISID_START, port, &psec_ifindex, __LINE__) != VTSS_RC_OK) {
        return;
    }

    {
        PSEC_LOCK_SCOPE();

        if (PSEC_interface_status_get(psec_ifindex.ifindex, &port_state, __LINE__) != VTSS_RC_OK) {
            return;
        }

        port_state.if_status.status.link_is_up = status->link;
        (void)PSEC_interface_status_set(psec_ifindex.ifindex, &port_state, __LINE__);

        PSEC_static_macs_handle(&psec_ifindex, &port_state, FALSE);
    }
}

#if defined(VTSS_SW_OPTION_MSTP)
/******************************************************************************/
// PSEC_stp_msti_state_change_callback()
/******************************************************************************/
static void PSEC_stp_msti_state_change_callback(vtss_common_port_t l2port, u8 msti, vtss_common_stpstate_t new_state)
{
    vtss_isid_t             isid;
    mesa_port_no_t          iport;
    psec_ifindex_t          psec_ifindex;
    psec_interface_status_t port_state;

    T_N("STP MSTI state change callback(l2port = %s, msti = %d, new_state = %d)", l2port2str(l2port), msti, new_state);

    if (!msg_switch_is_primary()) {
        return;
    }

    // Convert l2port to isid/iport
    if (!l2port2port(l2port, &isid, &iport)) {
        T_D("l2port2port() failed");
        return;
    }

    if (PSEC_ifindex_from_port(isid, iport, &psec_ifindex, __LINE__) != VTSS_RC_OK) {
        return;
    }

    {
        PSEC_LOCK_SCOPE();

        if (PSEC_interface_status_get(psec_ifindex.ifindex, &port_state, __LINE__) != VTSS_RC_OK) {
            return;
        }

        port_state.if_status.status.stp_discarding = new_state == VTSS_COMMON_STPSTATE_DISCARDING;

        if (new_state == VTSS_COMMON_STPSTATE_DISCARDING) {
            // Only remove the specific registered MAC addresses matching the specific STP MSTI
            PSEC_mac_del_by_msti(&psec_ifindex, &port_state, PSEC_DEL_REASON_PORT_STP_MSTI_DISCARDING, msti);
        }

        // Whether or not it's now discarding, we gotta check whether we need to re-enable
        // CPU copying on the port.
        PSEC_sec_learn_cpu_copy_check(&psec_ifindex, &port_state, PSEC_LEARN_CPU_REASON_OTHER, __LINE__);

        // Save any changes back to the state.
        (void)PSEC_interface_status_set(psec_ifindex.ifindex, &port_state, __LINE__);
    }
}
#endif /* VTSS_SW_OPTION_MSTP */

/******************************************************************************/
// PSEC_primary_switch_isid_port_check()
// Returns VTSS_RC_OK if we're the primary switch and isid and port are legal.
/******************************************************************************/
static mesa_rc PSEC_primary_switch_isid_port_check(vtss_isid_t isid, mesa_port_no_t port)
{
    if (!msg_switch_is_primary()) {
        return VTSS_APPL_PSEC_RC_MUST_BE_PRIMARY_SWITCH;
    }

    if (!VTSS_ISID_LEGAL(isid)) {
        return VTSS_APPL_PSEC_RC_INV_ISID;
    }

    if (port >= port_count_max()) {
        // Note that we do allow stack ports and port numbers higher than the isid's
        // because the API allows for configuring this module before a given
        // switch exists in the stack, and therefore we don't know the port count
        // of the switch or the stack ports of the switch at configuration time.
        return VTSS_APPL_PSEC_RC_INV_PORT;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// PSEC_do_age_or_hold()
/******************************************************************************/
static void PSEC_do_age_or_hold(void)
{
    char                    buf[PSEC_MAC_STR_BUF_SIZE];
    psec_mac_itr_t          mac_itr, mac_itr_next;
    psec_interface_status_t port_state;
    psec_ifindex_t          psec_ifindex;

    mac_itr = PSEC_mac_itr_get_first();

    while (mac_itr != psec_mac_map.end()) {
        // Keep a pointer to the next, because it may be that we delete the
        // current below.
        mac_itr_next = mac_itr;
        mac_itr_next++;

        if (mac_itr->second.status.age_or_hold_time_secs == 0 || mac_itr->second.status.kept_blocked) {
            goto next;
        }

        // Subject to aging/holding
        if (--mac_itr->second.status.age_or_hold_time_secs != 0) {
            goto next;
        }

        if (PSEC_ifindex_from_ifindex(mac_itr->second.status.ifindex, &psec_ifindex, __LINE__) != VTSS_RC_OK) {
            goto next;
        }

        if (PSEC_interface_status_get(mac_itr->second.status.ifindex, &port_state, __LINE__) != VTSS_RC_OK) {
            goto next;
        }

        (void)PSEC_mac_itr_to_str(mac_itr, buf);

        // Aging or holding timed out.
        if (mac_itr->second.hw_add_failed || mac_itr->second.sw_add_failed || mac_itr->second.status.blocked) {
            // It was due to hold time. Remove the entry
            T_I("%s: Hold-timeout", buf);
            PSEC_mac_del(&psec_ifindex, &port_state, mac_itr, PSEC_DEL_REASON_HOLD_TIME_EXPIRED, VTSS_APPL_PSEC_USER_LAST, VTSS_APPL_PSEC_USER_LAST);
        } else {
            // Aging timed out.
            if (mac_itr->second.status.age_frame_seen) {
                // A frame was seen during this aging period.
                // Keep entry, but re-enable CPU-copying and update the age time.
                // The following call will only update the age_or_hold_time_secs (due to the TRUE parameter)
                // If we had called the function with FALSE instead, then the function would have cleared
                // the CPU_COPYING flag, set the AGE_FRAME_SEEN flag and called the PSEC_mac_module_chg()
                // function itself. We want the opposite to happen.
                T_D("%s: Aging timeout. Age-frame seen", buf);
                (void)PSEC_mac_chg(&psec_ifindex, &port_state, mac_itr, TRUE);
                // So we need to restart aging ourselves.
                mac_itr->second.status.cpu_copying = TRUE;
                mac_itr->second.status.age_frame_seen = FALSE;
                (void)PSEC_mac_module_chg(&psec_ifindex, mac_itr, __LINE__);
            } else {
                // Aging timed out, but the station has not sent new frames in the aging period.
                // Unregister it.
                T_I("%s: Aging timeout. Age-frame NOT seen", buf);
                PSEC_mac_del(&psec_ifindex, &port_state, mac_itr, PSEC_DEL_REASON_AGED_OUT, VTSS_APPL_PSEC_USER_LAST, VTSS_APPL_PSEC_USER_LAST);
            }
        }

        // Save the changed state back
        (void)PSEC_interface_status_set(psec_ifindex.ifindex, &port_state, __LINE__);

next:
        mac_itr = mac_itr_next;
    }
}

/******************************************************************************/
// PSEC_callback_single_user_del()
/******************************************************************************/
static void PSEC_callback_single_user_del(psec_mac_add_del_itr_t mac_del_itr, vtss_appl_psec_user_t user)
{
    char buf[PSEC_MAC_STR_BUF_SIZE];

    // User cannot unregister - only enable/disable himself, so safe to deref
    // PSEC_state.on_mac_del_callbacks[].
    T_I("%s: Unregistering MAC @ user = %s", PSEC_vid_mac_to_str(mac_del_itr->first.ifindex, &mac_del_itr->second.members.vid_mac, buf), PSEC_user_name(user));
    PSEC_state.on_mac_del_callbacks[user](
        mac_del_itr->second.members.isid,
        mac_del_itr->second.members.port,
        &mac_del_itr->second.members.vid_mac,
        mac_del_itr->second.members.reason,
        mac_del_itr->second.members.forward_decision_del[user],
        mac_del_itr->second.members.originating_user_del);
}

/******************************************************************************/
// PSEC_callback_single_user_add()
/******************************************************************************/
static void PSEC_callback_single_user_add(psec_mac_add_del_itr_t mac_add_itr, vtss_appl_psec_user_t user)
{
    char buf[PSEC_MAC_STR_BUF_SIZE];

    (void)PSEC_vid_mac_to_str(mac_add_itr->first.ifindex, &mac_add_itr->second.members.vid_mac, buf);

    if (user == VTSS_APPL_PSEC_USER_ADMIN) {
        T_E("%s: This function is not supposed to be called for PSEC_LIMIT", buf);
        return;
    }

    // User cannot unregister - only enable/disable himself, so safe to deref
    // PSEC_state.on_mac_add_callbacks[].
    mac_add_itr->second.members.forward_decision_add[user] =
        PSEC_state.on_mac_add_callbacks[user](mac_add_itr->second.members.isid,
                                              mac_add_itr->second.members.port,
                                              &mac_add_itr->second.members.vid_mac,
                                              0,                         /* Only matters for PSEC Limit */
                                              VTSS_APPL_PSEC_USER_ADMIN, /* Only matters for PSEC Limit */
                                              NULL                       /* Only matters for PSEC Limit */);

    T_I("%s: Registering MAC @ user = %s => forward_decision = %s", buf, PSEC_user_name(user), psec_add_method_to_str(mac_add_itr->second.members.forward_decision_add[user]));
}

/******************************************************************************/
// PSEC_call_delayed()
// Callback users delayed. If an entry in @map both have
// users_to_call_delayed_del and users_to_call_delayed_add set, we call del
// before add.
/******************************************************************************/
static void PSEC_call_delayed(psec_mac_add_del_map_t &map)
{
    psec_mac_add_del_itr_t mac_add_del_itr;
    vtss_appl_psec_user_t  user_iter;

    T_I("Processing %zu MAC addresses delayed\n", map.size());

    for (mac_add_del_itr = map.begin(); mac_add_del_itr != map.end(); ++mac_add_del_itr) {
        for (user_iter = (vtss_appl_psec_user_t)0; user_iter < VTSS_APPL_PSEC_USER_LAST; user_iter++) {
            // First delete
            if (PSEC_USER_CALL_DELAYED_DEL_GET(mac_add_del_itr, user_iter)) {
                PSEC_callback_single_user_del(mac_add_del_itr, user_iter);
            }

            // Then add
            if (PSEC_USER_CALL_DELAYED_ADD_GET(mac_add_del_itr, user_iter)) {
                PSEC_callback_single_user_add(mac_add_del_itr, user_iter);
            }
        }
    }
}

/******************************************************************************/
// PSEC_xfer_delayed_add_results_single_user()
/******************************************************************************/
static void PSEC_xfer_delayed_add_results_single_user(psec_mac_add_del_itr_t mac_add_itr, vtss_appl_psec_user_t user)
{
    char                         buf[PSEC_MAC_STR_BUF_SIZE];
    psec_ifindex_t               psec_ifindex;
    psec_interface_status_t      port_state;
    psec_mac_itr_t               mac_itr         = PSEC_mac_itr_get(&mac_add_itr->first);
    psec_add_method_t            user_add_method = mac_add_itr->second.members.forward_decision_add[user];

    (void)PSEC_vid_mac_to_str(mac_add_itr->first.ifindex, &mac_add_itr->second.members.vid_mac, buf);

    if (mac_itr == psec_mac_map.end() || mac_add_itr->second.members.unique != mac_itr->second.unique) {
        // Someone deleted this MAC entry or changed it while we had our mutex exited. Nothing more to do
        T_I("%s: Skipping, because unique has changed", buf);
        return;
    }

    T_I("%s: User = %s: Changed add-method from %s to %s",
        buf,
        PSEC_user_name(user),
        psec_add_method_to_str(mac_itr->second.forward_decision[user]),
        psec_add_method_to_str(user_add_method));

    if (user_add_method == mac_itr->second.forward_decision[user]) {
        // User doesn't want to change the decision taken by us when we
        // added the MAC address in the first place.
        return;
    }

    if (PSEC_interface_status_get(mac_add_itr->first.ifindex, &port_state, __LINE__) != VTSS_RC_OK) {
        return;
    }

    mac_itr->second.forward_decision[user] = user_add_method;

    if (PSEC_ifindex_from_ifindex(mac_itr->first.ifindex, &psec_ifindex, __LINE__) != VTSS_RC_OK) {
        return;
    }

    // If we get here, we should try to add the MAC address to the MAC module.
    // This function will also determine how the MAC address should be added,
    // i.e. blocked or forwarding, based on the user-decisions, and it will
    // determine the aging/hold.
    (void)PSEC_mac_chg(&psec_ifindex, &port_state, mac_itr, FALSE);

    // Update the cpu-copy enable/disable state
    PSEC_sec_learn_cpu_copy_check(&psec_ifindex, &port_state, PSEC_LEARN_CPU_REASON_OTHER, __LINE__);

    (void)PSEC_interface_status_set(mac_add_itr->first.ifindex, &port_state, __LINE__);
}

/******************************************************************************/
// PSEC_xfer_delayed_add_results()
/******************************************************************************/
static void PSEC_xfer_delayed_add_results(psec_mac_add_del_map_t &map)
{
    psec_mac_add_del_itr_t mac_add_itr;
    vtss_appl_psec_user_t  user_iter;

    PSEC_CRIT_ASSERT_LOCKED();

    for (mac_add_itr = map.begin(); mac_add_itr != map.end(); ++mac_add_itr) {
        for (user_iter = (vtss_appl_psec_user_t)0; user_iter < VTSS_APPL_PSEC_USER_LAST; user_iter++) {
            if (!PSEC_USER_CALL_DELAYED_ADD_GET(mac_add_itr, user_iter)) {
                continue;
            }

            PSEC_xfer_delayed_add_results_single_user(mac_add_itr, user_iter);
        }
    }
}

/******************************************************************************/
// PSEC_thread()
// This thread takes care of aging and holding entries as well as calling back
// enabled PSEC-users (except for PSEC_LIMIT/ADMIN, which is called back
// synchronously) without the PSEC mutex taken. This in order to avoid
// deadlocks.
/******************************************************************************/
static void PSEC_thread(vtss_addrword_t data)
{
    psec_mac_add_del_map_t auto_mac_add_del_map;
    u64                    next_timeout_ms, now_ms;
    bool                   timed_out;

    while (1) {
        if (msg_switch_is_primary()) {
            // We should timeout every one second (1000 ms)
            next_timeout_ms = vtss::uptime_milliseconds() + 1000;

            while (msg_switch_is_primary()) {
                if (!msg_switch_is_primary()) {
                    break;
                }

                (void)vtss_flag_timed_wait(&PSEC_thread_wait_flag, 0xFFFFFFFF, VTSS_FLAG_WAITMODE_OR_CLR, VTSS_OS_MSEC2TICK(next_timeout_ms));
                now_ms = vtss::uptime_milliseconds();

                if (now_ms >= next_timeout_ms) {
                    timed_out = true;
                    next_timeout_ms += 1000;
                } else {
                    timed_out = false;
                }

                PSEC_CRIT_ENTER();

                if (timed_out) {
                    PSEC_do_age_or_hold();
                }

                // Move current add/del map (empty as well as non-empty) to a
                // local map, we can process without our mutex taken.
                auto_mac_add_del_map = vtss::move(psec_mac_add_del_map);

                PSEC_CRIT_EXIT();

                if (auto_mac_add_del_map.size() > 0) {
                    PSEC_call_delayed(auto_mac_add_del_map);

                    // We have to transfer the results of add operations back
                    // to the global map.
                    PSEC_CRIT_ENTER();
                    PSEC_xfer_delayed_add_results(auto_mac_add_del_map);
                    PSEC_CRIT_EXIT();

                    auto_mac_add_del_map.clear();
                }
            }
        }

        // No longer primary switch. Time to bail out.
        // No reason for using CPU ressources when we're a secondary switch
        T_D("Suspending PSEC thread");
        msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_PSEC);
        T_D("Resumed PSEC thread");
    }
}

/******************************************************************************/
// PSEC_port_mode_update()
// Updates two internal variables that control what happens when a limit is
// reached and whether anybody else but a given user module may add MAC
// addresses.
/******************************************************************************/
static void PSEC_port_mode_update(vtss_appl_psec_user_t user, psec_ifindex_t *psec_ifindex, psec_interface_status_t *port_state)
{
    vtss_appl_psec_user_t user_iter;

    port_state->if_status.block_learn_frames       = FALSE;
    port_state->if_status.keep_cpu_copying_enabled = FALSE;

    // Loop through all enabled users and update the port state
    for (user_iter = (vtss_appl_psec_user_t)0; user_iter < VTSS_APPL_PSEC_USER_LAST; user_iter++) {
        if (!PSEC_USER_ENA_GET(port_state, user_iter)) {
            continue;
        }

        switch (port_state->if_status.port_mode[user_iter]) {
        case PSEC_PORT_MODE_KEEP_BLOCKED:
            port_state->if_status.block_learn_frames = TRUE;
            break;

        case PSEC_PORT_MODE_RESTRICT:
            port_state->if_status.keep_cpu_copying_enabled = TRUE;
            break;

        case PSEC_PORT_MODE_NORMAL:
        case PSEC_PORT_MODE_LAST:
        default:
            break;
        }
    }

    // If there are no more users of keep_cpu_copying_enabled, clear
    // the total violate counter along with the latest_violating_mac and VLAN.
    // The current violate counter gets cleared along with deleting the MAC addresses.
    // Also clear it if psec_limit changes configuration. Otherwise it wouldn't
    // get cleared if he goes from 'restrict' to 'shutdown', since the mode is the same.
    if (user == VTSS_APPL_PSEC_USER_ADMIN || !port_state->if_status.keep_cpu_copying_enabled) {
        port_state->notif_status.total_violate_cnt = 0;
        port_state->notif_status.latest_violating_vlan = 0;
        memset(&port_state->notif_status.latest_violating_mac, 0, sizeof(port_state->notif_status.latest_violating_mac));
    }
}

/****************************************************************************/
/*                                                                          */
/*  SEMI-PUBLIC FUNCTIONS                                                   */
/*  These functions are meant for other modules to use, not for admin.    . */
/*                                                                          */
/****************************************************************************/

/******************************************************************************/
// psec_mgmt_time_conf_set_special()
/******************************************************************************/
mesa_rc psec_mgmt_time_conf_set_special(vtss_appl_psec_user_t user, u32 aging_period_secs, u32 hold_time_secs)
{
    vtss_isid_t isid;

    if (user >= VTSS_APPL_PSEC_USER_LAST) {
        return VTSS_APPL_PSEC_RC_INV_USER;
    }

    if (aging_period_secs != 0 && (aging_period_secs < PSEC_AGE_TIME_MIN || aging_period_secs > PSEC_AGE_TIME_MAX)) {
        return VTSS_APPL_PSEC_RC_INV_AGING_PERIOD;
    }

    if (hold_time_secs < PSEC_HOLD_TIME_MIN || hold_time_secs > PSEC_HOLD_TIME_MAX) {
        return VTSS_APPL_PSEC_RC_INV_HOLD_TIME;
    }

    PSEC_CRIT_ASSERT_LOCKED();

    if (PSEC_state.aging_period_secs[user] == aging_period_secs &&
        PSEC_state.hold_time_secs[user]    == hold_time_secs) {
        // No change
        return VTSS_RC_OK;
    }

    PSEC_state.aging_period_secs[user] = aging_period_secs;
    PSEC_state.hold_time_secs[user]    = hold_time_secs;

    // Now check if this affects any of the already registered MAC addresses.
    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        port_iter_t pit;

        if (!PSEC_switch_exists[isid - VTSS_ISID_START]) {
            continue;
        }

        (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            psec_ifindex_t          psec_ifindex;
            psec_interface_status_t port_state;
            psec_mac_itr_t          mac_itr;

            if (PSEC_ifindex_from_port(isid, pit.iport, &psec_ifindex, __LINE__) != VTSS_RC_OK) {
                continue;
            }

            if (PSEC_interface_status_get(psec_ifindex.ifindex, &port_state, __LINE__) != VTSS_RC_OK) {
                continue;
            }

            if (!PSEC_USER_ENA_GET(&port_state, user)) {
                continue;
            }

            for (mac_itr = PSEC_mac_itr_get_first_from_ifindex(psec_ifindex.ifindex); mac_itr != psec_mac_map.end(); ++mac_itr) {
                // The PSEC_mac_chg() function will update the age and hold times of
                // the running counter.
                // The TRUE indicates that we don't want to change the forward
                // decision, but only the hold or age time based on the currently
                // enabled users and their forwarding decision.
                if (!PSEC_mac_chg(&psec_ifindex, &port_state, mac_itr, TRUE)) {
                    T_E("Internal error");
                    return VTSS_RC_ERROR;
                }
            }

            // The port_state has not been altered, so no need to save back the state.
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// psec_mgmt_time_conf_set()
/******************************************************************************/
mesa_rc psec_mgmt_time_conf_set(vtss_appl_psec_user_t user, u32 aging_period_secs, u32 hold_time_secs)
{
    mesa_rc rc;

    if (user >= VTSS_APPL_PSEC_USER_LAST || user == VTSS_APPL_PSEC_USER_ADMIN) {
        // PSEC_LIMIT should use psec_mgmt_time_conf_set_special()
        return VTSS_APPL_PSEC_RC_INV_USER;
    }

    PSEC_CRIT_ENTER();
    rc = psec_mgmt_time_conf_set_special(user, aging_period_secs, hold_time_secs);
    PSEC_CRIT_EXIT();
    return rc;
}

/******************************************************************************/
// psec_mgmt_port_conf_set_special()
/******************************************************************************/
mesa_rc psec_mgmt_port_conf_set_special(vtss_appl_psec_user_t user,
                                        vtss_isid_t           isid,
                                        mesa_port_no_t        port,
                                        BOOL                  enable,
                                        psec_port_mode_t      port_mode,
                                        BOOL                  reopen_port,
                                        BOOL                  limit_reached,
                                        u32                   violate_limit,
                                        BOOL                  sticky)
{
    psec_ifindex_t          psec_ifindex;
    psec_interface_status_t port_state;
    BOOL                    first_user;

    // If invoked directly by us, we must already have taken the PSEC mutex.
    // If invoked by PSEC_LIMIT, it must also have taken it.
    PSEC_CRIT_ASSERT_LOCKED();

    T_I("%u:%u, user = %s, enable = %d, reopen_port = %d, limit_reached = %d, port_mode = %d", isid, port, PSEC_user_name(user), enable, reopen_port, limit_reached, port_mode);

    if (user >= VTSS_APPL_PSEC_USER_LAST) {
        return VTSS_APPL_PSEC_RC_INV_USER;
    }

    if (port_mode >= PSEC_PORT_MODE_LAST) {
        return VTSS_APPL_PSEC_RC_INV_PARAM;
    }

    VTSS_RC(PSEC_primary_switch_isid_port_check(isid, port));
    VTSS_RC(PSEC_ifindex_from_port(isid, port, &psec_ifindex, __LINE__));
    VTSS_RC(PSEC_interface_status_get(psec_ifindex.ifindex, &port_state, __LINE__));

    first_user = port_state.if_status.status.users == 0;
    PSEC_USER_ENA_SET(&port_state, user, enable);

    // Set the user's preferred Secure Learning CPU copy method.
    port_state.if_status.port_mode[user] = port_mode;
    PSEC_port_mode_update(user, &psec_ifindex, &port_state);

    if (user == VTSS_APPL_PSEC_USER_ADMIN) {
        // Only PSEC_LIMIT may change the violate limit.
        port_state.if_status.violate_limit = violate_limit;
        port_state.if_status.status.sticky = sticky;
    }

    if (enable) {
        // A (new) user wants to have something to say on this port.
        // We do that by deleting all currently known MAC addresses on the
        // port and start all over - except for perhaps reopening after a shutdown.
        PSEC_mac_del_all(&psec_ifindex, &port_state, PSEC_DEL_REASON_USER_DELETED, user, TRUE);

        if (first_user) {
            // This is the first user to enable on this port. Send port-configuration.
            PSEC_msg_tx_port_conf(isid, port, TRUE);
        }
    } else {
        // The user wants to back out.
        if (port_state.if_status.status.users) {
            // If there are still enabled users on this port, disabling one user may cause
            // blocked MAC addresses to become unblocked.
            // Loop through all entries attached to this port and check if they still need
            // to be blocked or can be unblocked.
            psec_mac_itr_t mac_itr;

            for (mac_itr = PSEC_mac_itr_get_first_from_ifindex(psec_ifindex.ifindex); mac_itr != psec_mac_map.end(); ++mac_itr) {
                if (user == VTSS_APPL_PSEC_USER_ADMIN) {
                    // Back to dynamic (if not already dynamic).
                    mac_itr->second.status.mac_type = VTSS_APPL_PSEC_MAC_TYPE_DYNAMIC;
                }

                // The FALSE indicates that we also want to change the forwarding decision,
                // and not only the age and hold time.
                (void)PSEC_mac_chg(&psec_ifindex, &port_state, mac_itr, FALSE);
            }
        } else {
            // This is the last user enabled on this port. Send port-configuration.
            PSEC_msg_tx_port_conf(isid, port, FALSE);

            // There are no more enabled users on this port. Remove all MAC addresses learned
            PSEC_mac_del_all(&psec_ifindex, &port_state, PSEC_DEL_REASON_NO_MORE_USERS, VTSS_APPL_PSEC_USER_LAST, TRUE);

            // And disable secure learning.
            PSEC_sec_learn_cpu_copy_check(&psec_ifindex, &port_state, PSEC_LEARN_CPU_REASON_OTHER, __LINE__);
        }
    }

    if (reopen_port) {
        // Should only be set by PSEC LIMIT module. If the limit was reached or the
        // port was shut down, then it should be re-opened when the PSEC LIMIT disables.
        // This could have been done by checking whether user == VTSS_APPL_PSEC_USER_ADMIN,
        // but in order to support future security modules, we have it in the API function,
        // so that we only need to change this file in case more than one module can set this
        // parameter.
        port_state.if_status.status.limit_reached = FALSE;
        port_state.notif_status.shut_down = FALSE;
        PSEC_shutdown_set(&psec_ifindex, FALSE);
    }

    if (enable && limit_reached) {
        // Should only be set by PSEC LIMIT module, and only when enabling the port
        // with a limit of 0.
        port_state.if_status.status.limit_reached = TRUE;
    }

    // Check whether we should copy learn frames to CPU
    PSEC_sec_learn_cpu_copy_check(&psec_ifindex, &port_state, PSEC_LEARN_CPU_REASON_OTHER, __LINE__);

    // Save the new state back
    (void)PSEC_interface_status_set(psec_ifindex.ifindex, &port_state, __LINE__);

    if (enable) {
        // In case we just deleted all MAC addresses (we do that whenever any
        // user module enables itself), we gotta check whether we need to add
        // any sticky/static MACs.
        PSEC_static_macs_handle(&psec_ifindex, &port_state, user == VTSS_APPL_PSEC_USER_ADMIN);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// psec_mgmt_port_conf_set()
/******************************************************************************/
mesa_rc psec_mgmt_port_conf_set(vtss_appl_psec_user_t user,
                                vtss_isid_t           isid,
                                mesa_port_no_t        port,
                                BOOL                  enable,
                                psec_port_mode_t      port_mode)
{
    if (user >= VTSS_APPL_PSEC_USER_LAST || user == VTSS_APPL_PSEC_USER_ADMIN) {
        // PSEC_LIMIT should call psec_mgmt_port_conf_set_special() directly.
        return VTSS_APPL_PSEC_RC_INV_USER;
    }

    if (port_mode >= PSEC_PORT_MODE_LAST) {
        return VTSS_APPL_PSEC_RC_INV_PARAM;
    }

    {
        PSEC_LOCK_SCOPE();
        return psec_mgmt_port_conf_set_special(user, isid, port, enable, port_mode, FALSE, FALSE, 0, FALSE);
    }
}

/******************************************************************************/
// psec_mgmt_port_sticky_set()
// Called by PSEC_LIMIT whenever sticky flag changes on a port.
// Purpose is to change all dynamic entries to sticky and vice versa.
/******************************************************************************/
mesa_rc psec_mgmt_port_sticky_set(vtss_ifindex_t ifindex, BOOL sticky, psec_on_mac_sticky_change_f *on_mac_sticky_change_callback)
{
    psec_interface_status_t port_state;
    psec_mac_itr_t          mac_itr;

    // This is locked by PSEC_LIMIT module.
    PSEC_CRIT_ASSERT_LOCKED();

    VTSS_RC(PSEC_interface_status_get(ifindex, &port_state, __LINE__));
    port_state.if_status.status.sticky = sticky;

    // Loop through all MAC entries and change sticky to dynamic or vice
    // versa. Also change aging as appropriate.
    for (mac_itr = PSEC_mac_itr_get_first_from_ifindex(ifindex); mac_itr != psec_mac_map.end(); ++mac_itr) {
        BOOL changed = FALSE;

        if (mac_itr->first.ifindex > ifindex) {
            break;
        }

        if (mac_itr->second.status.violating) {
            // Skip violating entries
            continue;
        }

        if (sticky) {
            if (mac_itr->second.status.mac_type == VTSS_APPL_PSEC_MAC_TYPE_DYNAMIC) {
                mac_itr->second.status.mac_type = VTSS_APPL_PSEC_MAC_TYPE_STICKY;
                changed = TRUE;
            }
        } else {
            if (mac_itr->second.status.mac_type == VTSS_APPL_PSEC_MAC_TYPE_STICKY) {
                mac_itr->second.status.mac_type = VTSS_APPL_PSEC_MAC_TYPE_DYNAMIC;
                changed = TRUE;
            }
        }

        if (changed) {
            // Update age times of the running counter.
            // The TRUE indicates that we don't want to change the forward
            // decision, but only the hold or age time based on the
            // currently enabled users and their forward decision and
            // stickiness.
            psec_ifindex_t psec_ifindex;
            VTSS_RC(PSEC_ifindex_from_ifindex(ifindex, &psec_ifindex, __LINE__));

            if (!PSEC_mac_chg(&psec_ifindex, &port_state, mac_itr, TRUE)) {
                T_E("Internal Error");
            }

            if (on_mac_sticky_change_callback) {
                vtss_appl_psec_mac_conf_t mac_conf;
                PSEC_vid_mac_to_mac_conf(&mac_itr->second.status.vid_mac, &mac_conf, mac_itr->second.status.mac_type);
                on_mac_sticky_change_callback(ifindex, &mac_conf);
            }
        }
    }

    // Save the new state back
    (void)PSEC_interface_status_set(ifindex, &port_state, __LINE__);

    return VTSS_RC_OK;
}

/******************************************************************************/
// psec_mgmt_mac_chg()
/******************************************************************************/
mesa_rc psec_mgmt_mac_chg(vtss_appl_psec_user_t user, vtss_isid_t isid, mesa_port_no_t port, mesa_vid_mac_t *vid_mac, psec_add_method_t new_method)
{
    char                         buf[PSEC_MAC_STR_BUF_SIZE];
    psec_interface_status_t      port_state;
    psec_mac_itr_t               mac_itr;
    vtss_appl_psec_mac_map_key_t key;
    psec_ifindex_t               psec_ifindex;

    if (user >= VTSS_APPL_PSEC_USER_LAST) {
        return VTSS_APPL_PSEC_RC_INV_USER;
    }

    VTSS_RC(PSEC_primary_switch_isid_port_check(isid, port));
    VTSS_RC(PSEC_ifindex_from_port(isid, port, &psec_ifindex, __LINE__));

    T_D("%s: User %s: Changing method to %s", PSEC_vid_mac_to_str(psec_ifindex.ifindex, vid_mac, buf), PSEC_user_name(user), psec_add_method_to_str(new_method));

    {
        PSEC_LOCK_SCOPE();

        memset(&key, 0, sizeof(key));
        key.ifindex = psec_ifindex.ifindex;
        key.vlan    = vid_mac->vid;
        key.mac     = vid_mac->mac;

        if ((mac_itr = PSEC_mac_itr_get(&key)) == psec_mac_map.end()) {
            return VTSS_APPL_PSEC_RC_MAC_VID_NOT_FOUND;
        }

        // In reality we should also check if this user is enabled on this port, but
        // since it doesn't change anything in the forward decision if he isn't, we
        // don't care.

        // Set the user's new forward decision
        mac_itr->second.forward_decision[user] = new_method;

        VTSS_RC(PSEC_interface_status_get(psec_ifindex.ifindex, &port_state, __LINE__));

        // This may give rise to another forwarding decision for this MAC address.
        // The FALSE indicates that we also want to change the forwarding decision,
        // and not only the age and hold time.
        (void)PSEC_mac_chg(&psec_ifindex, &port_state, mac_itr, FALSE);

        // No need to re-investigate whether the secure learning/CPU copy should be altered.
        // And no need to save back the port_state, because it's not altered here.
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// psec_mgmt_mac_add()
/******************************************************************************/
mesa_rc psec_mgmt_mac_add(vtss_appl_psec_user_t user, vtss_isid_t isid, mesa_port_no_t port, mesa_vid_mac_t *vid_mac, psec_add_method_t method)
{
    char                      buf[PSEC_MAC_STR_BUF_SIZE];
    psec_interface_status_t   port_state;
    psec_mac_itr_t            mac_itr;
    psec_ifindex_t            psec_ifindex;
    mesa_rc                   rc;
    vtss_appl_psec_mac_conf_t mac_conf;

    if (user >= VTSS_APPL_PSEC_USER_LAST) {
        return VTSS_APPL_PSEC_RC_INV_USER;
    }

    // Only unicast MAC addresses!
    if (!PSEC_mac_is_unicast(&vid_mac->mac)) {
        return VTSS_APPL_PSEC_RC_MAC_NOT_UNICAST;
    }

    VTSS_RC(PSEC_primary_switch_isid_port_check(isid, port));
    VTSS_RC(PSEC_ifindex_from_port(isid, port, &psec_ifindex, __LINE__));
    PSEC_vid_mac_to_mac_conf(vid_mac, &mac_conf, VTSS_APPL_PSEC_MAC_TYPE_DYNAMIC);

    (void)PSEC_mac_conf_to_str(psec_ifindex.ifindex, &mac_conf, buf);
    T_D("%s", buf);

    {
        mesa_vid_t fid;
        PSEC_LOCK_SCOPE();

        fid = psec_limit_mgmt_fid_get(vid_mac->vid);

        if ((mac_itr = PSEC_mac_itr_get_from_fid_mac(fid, &vid_mac->mac)) != psec_mac_map.end()) {
            T_W("%s: Already found on ifindex = %u, FID = %u", buf, VTSS_IFINDEX_PRINTF_ARG(mac_itr->first.ifindex), fid);
            return mac_itr->first.vlan == vid_mac->vid ? VTSS_APPL_PSEC_RC_MAC_VID_ALREADY_FOUND : VTSS_APPL_PSEC_RC_MAC_VID_ALREADY_FOUND_ON_SVL;
        }

        VTSS_RC(PSEC_interface_status_get(psec_ifindex.ifindex, &port_state, __LINE__));

        // Only users that have proclaimed that the port should remain in non-CPU-copy mode are allowed to call this function
        if (port_state.if_status.port_mode[user] != PSEC_PORT_MODE_KEEP_BLOCKED || !PSEC_USER_ENA_GET(&port_state, user)) {
            T_E("%d:%d: Called by user (%s) that hasn't proclaimed correct port mode", isid, iport2uport(port), PSEC_user_name(user));
            return VTSS_APPL_PSEC_RC_INV_USER_MODE;
        }

        mac_conf.mac_type = port_state.if_status.status.sticky ? VTSS_APPL_PSEC_MAC_TYPE_DYNAMIC : VTSS_APPL_PSEC_MAC_TYPE_STICKY;
        rc = PSEC_mac_add(&psec_ifindex, &port_state, &mac_conf, method, user, FALSE);

        // Save back the new port_state whether or not PSEC_mac_add() succeeded.
        (void)PSEC_interface_status_set(psec_ifindex.ifindex, &port_state, __LINE__);
    }

    return rc;
}

/******************************************************************************/
// psec_mgmt_mac_del()
/******************************************************************************/
mesa_rc psec_mgmt_mac_del(vtss_appl_psec_user_t user, vtss_isid_t isid, mesa_port_no_t port, mesa_vid_mac_t *vid_mac)
{
    psec_interface_status_t      port_state;
    psec_mac_itr_t               mac_itr;
    psec_ifindex_t               psec_ifindex;
    vtss_appl_psec_mac_map_key_t key;
    char                         buf[PSEC_MAC_STR_BUF_SIZE];

    if (user >= VTSS_APPL_PSEC_USER_LAST) {
        return VTSS_APPL_PSEC_RC_INV_USER;
    }

    if (!vid_mac) {
        return VTSS_APPL_PSEC_RC_INV_PARAM;
    }

    VTSS_RC(PSEC_primary_switch_isid_port_check(isid, port));
    VTSS_RC(PSEC_ifindex_from_port(isid, port, &psec_ifindex, __LINE__));

    (void)PSEC_vid_mac_to_str(psec_ifindex.ifindex, vid_mac, buf);
    T_D("%s", buf);

    {
        PSEC_LOCK_SCOPE();

        memset(&key, 0, sizeof(key));
        key.ifindex = psec_ifindex.ifindex;
        key.vlan    = vid_mac->vid;
        key.mac     = vid_mac->mac;

        if ((mac_itr = PSEC_mac_itr_get(&key)) == psec_mac_map.end()) {
            return VTSS_APPL_PSEC_RC_MAC_VID_NOT_FOUND;
        }

        VTSS_RC(PSEC_interface_status_get(psec_ifindex.ifindex, &port_state, __LINE__));

        // Only users that have proclaimed that the port should remain in non-CPU-copy mode are allowed to call this function.
        if (port_state.if_status.port_mode[user] != PSEC_PORT_MODE_KEEP_BLOCKED || !PSEC_USER_ENA_GET(&port_state, user)) {
            T_E("%s: Called by user (%s) that hasn't proclaimed correct port mode", buf, PSEC_user_name(user));
            return VTSS_APPL_PSEC_RC_INV_USER_MODE;
        }

        PSEC_mac_del(&psec_ifindex, &port_state, mac_itr, PSEC_DEL_REASON_USER_DELETED, user, user);

        // Save back the new port_state
        (void)PSEC_interface_status_set(psec_ifindex.ifindex, &port_state, __LINE__);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// psec_mgmt_mac_add_special()
// This function is only supposed to be invoked by PSEC_LIMIT.
/******************************************************************************/
mesa_rc psec_mgmt_mac_add_special(vtss_appl_psec_user_t user, vtss_ifindex_t ifindex, vtss_appl_psec_mac_conf_t *mac_conf)
{
    char                    buf[PSEC_MAC_STR_BUF_SIZE];
    psec_interface_status_t port_state;
    psec_mac_itr_t          mac_itr;
    psec_ifindex_t          psec_ifindex;
    mesa_vid_t              fid;
    mesa_rc                 rc;

    if (user != VTSS_APPL_PSEC_USER_ADMIN) {
        return VTSS_APPL_PSEC_RC_INV_USER;
    }

    if (!mac_conf) {
        return VTSS_APPL_PSEC_RC_INV_PARAM;
    }

    // Only unicast MAC addresses!
    if (!PSEC_mac_is_unicast(&mac_conf->mac)) {
        return VTSS_APPL_PSEC_RC_MAC_NOT_UNICAST;
    }

    (void)PSEC_mac_conf_to_str(ifindex, mac_conf, buf);
    T_D("%s", buf);

    // The PSEC_LIMIT module must have taken our mutex prior to calling us.
    PSEC_CRIT_ASSERT_LOCKED();

    VTSS_RC(PSEC_ifindex_from_ifindex(ifindex, &psec_ifindex, __LINE__));
    fid = psec_limit_mgmt_fid_get(mac_conf->vlan);

    if ((mac_itr = PSEC_mac_itr_get_from_fid_mac(fid, &mac_conf->mac)) != psec_mac_map.end()) {
        psec_ifindex_t other_psec_ifindex;

        // Entry already found.
        // If the entry is dynamic, we simply remove it silently to make room
        // for this new static or sticky one.
        // If the entry is not dynamic, there's a bug somewhere - probably in
        // PSEC_LIMIT, because it should know that it's already added.
        VTSS_RC(PSEC_ifindex_from_ifindex(mac_itr->first.ifindex, &other_psec_ifindex, __LINE__));

        T_I("%s: Already found on ifindex = %u mapping to FID = %u", buf, VTSS_IFINDEX_PRINTF_ARG(other_psec_ifindex.ifindex), fid);

        if (mac_itr->second.status.mac_type == VTSS_APPL_PSEC_MAC_TYPE_DYNAMIC) {
            VTSS_RC(PSEC_interface_status_get(other_psec_ifindex.ifindex, &port_state, __LINE__));

            // Remove old entry silently, but let everyone know it.
            PSEC_mac_del(&other_psec_ifindex, &port_state, mac_itr, PSEC_DEL_REASON_STATION_MOVED, VTSS_APPL_PSEC_USER_LAST, user);

            (void)PSEC_interface_status_set(other_psec_ifindex.ifindex, &port_state, __LINE__);
        } else if (mac_itr->first.vlan != mac_conf->vlan) {
            // Entry is not dynamic, so it's added by PSEC_LIMIT. However, it
            // shouldn't allow adding a <MAC, VID>, where the VID map to the
            // same FID as another non-dynamic entry.
            T_E("%s: Already found on ifindex = %u, VLAN %u, which maps to the same FID = %u", buf, VTSS_IFINDEX_PRINTF_ARG(other_psec_ifindex.ifindex), mac_itr->first.vlan, fid);

            return VTSS_APPL_PSEC_RC_MAC_VID_ALREADY_FOUND_ON_SVL;
        } else if (other_psec_ifindex.ifindex != psec_ifindex.ifindex) {
            // Entry is not dynamic, so there's a bug in PSEC_LIMIT, we presume,
            // since it now attempts to add it on another interface.
            T_E("%s: Already found on ifindex = %u", buf, VTSS_IFINDEX_PRINTF_ARG(other_psec_ifindex.ifindex));

            return VTSS_APPL_PSEC_RC_MAC_VID_ALREADY_FOUND;
        } else {
            // It's adding it on the same interface as it has added it before.
            // This is fine, because psec_limit_mgmt_static_macs_get() may be
            // called multiple times for the same interface. What is important
            // is that we don't really try to add it again.
            return VTSS_RC_OK;
        }
    }

    VTSS_RC(PSEC_interface_status_get(psec_ifindex.ifindex, &port_state, __LINE__));

    // PSEC_LIMIT must be called back, so last arg must be VTSS_APPL_PSEC_USER_LAST
    rc = PSEC_mac_add(&psec_ifindex, &port_state, mac_conf, PSEC_ADD_METHOD_CNT, VTSS_APPL_PSEC_USER_LAST, TRUE /* Avoid calling back other users (excl. PSEC_LIMIT) until all mutexes are exited */);

    // Save back the new port_state whether or not PSEC_mac_add() succeeded.
    (void)PSEC_interface_status_set(psec_ifindex.ifindex, &port_state, __LINE__);

    return rc;
}

/******************************************************************************/
// psec_mgmt_mac_del_special()
/******************************************************************************/
mesa_rc psec_mgmt_mac_del_special(vtss_appl_psec_user_t user, vtss_ifindex_t ifindex, vtss_appl_psec_mac_conf_t *mac_conf)
{
    psec_interface_status_t      port_state;
    psec_mac_itr_t               mac_itr;
    psec_ifindex_t               psec_ifindex;
    vtss_appl_psec_mac_map_key_t key;
    char                         buf[PSEC_MAC_STR_BUF_SIZE];

    if (user != VTSS_APPL_PSEC_USER_ADMIN) {
        return VTSS_APPL_PSEC_RC_INV_USER;
    }

    if (!mac_conf) {
        return VTSS_APPL_PSEC_RC_INV_PARAM;
    }

    T_D("%s", PSEC_mac_conf_to_str(ifindex, mac_conf, buf));

    // The PSEC_LIMIT module must have taken our mutex prior to calling us.
    PSEC_CRIT_ASSERT_LOCKED();

    VTSS_RC(PSEC_ifindex_from_ifindex(ifindex, &psec_ifindex, __LINE__));

    memset(&key, 0, sizeof(key));
    key.ifindex = psec_ifindex.ifindex;
    key.vlan    = mac_conf->vlan;
    key.mac     = mac_conf->mac;

    if ((mac_itr = PSEC_mac_itr_get(&key)) == psec_mac_map.end()) {
        return VTSS_APPL_PSEC_RC_MAC_VID_NOT_FOUND;
    }

    VTSS_RC(PSEC_interface_status_get(psec_ifindex.ifindex, &port_state, __LINE__));

    // Make sure PSEC_LIMIT also gets to know about this.
    PSEC_mac_del(&psec_ifindex, &port_state, mac_itr, PSEC_DEL_REASON_USER_DELETED, VTSS_APPL_PSEC_USER_LAST, user);

    // Save back the new port_state
    (void)PSEC_interface_status_set(psec_ifindex.ifindex, &port_state, __LINE__);

    return VTSS_RC_OK;
}

/******************************************************************************/
// psec_mgmt_mutex_enter()
/******************************************************************************/
void psec_mgmt_mutex_enter(void)
{
    PSEC_CRIT_ENTER();
}

/******************************************************************************/
// psec_mgmt_mutex_exit()
/******************************************************************************/
void psec_mgmt_mutex_exit(void)
{
    PSEC_CRIT_EXIT();
}

/******************************************************************************/
// psec_mgmt_fid_change()
// This function is called by the PSEC_LIMIT module whenever an SVL mapping
// changes. The purpose is to go through all <vid, *> entries and change them in
// the MAC table from old_fid to new_fid.
//
// Changing the FID can cause two existing <VID, MAC> entries to start
// representing the same entry in the MAC table, because they are learned on a
// FID. If this happens, one of them is silently deleted.
//
// The reason that this function is not a callback from the VLAN module, but
// rather a direct invocation by the PSEC_LIMIT module is that the PSEC_LIMIT
// module must clean up all its static/sticky entries first, before we can
// clean up the dynamic ones (which the PSEC_LIMIT module doesn't know anything
// about). If this function was a callback from the VLAN module, we wouldn't
// know the order of callbacks.
//
// When this function is invoked, the PSEC mutex is already taken.
/******************************************************************************/
void psec_mgmt_fid_change(mesa_vid_t vid, mesa_vid_t old_fid, mesa_vid_t new_fid)
{
    char                    buf[PSEC_MAC_STR_BUF_SIZE], buf2[PSEC_MAC_STR_BUF_SIZE];
    psec_mac_itr_t          mac_itr, mac_itr_next, tmp_mac_itr;
    psec_ifindex_t          psec_ifindex;
    psec_interface_status_t port_state;
    mesa_mac_table_entry_t  entry;
    mesa_rc                 rc;

    PSEC_CRIT_ASSERT_LOCKED();

    T_I("VID = %u: FID change from %u to %u", vid, old_fid, new_fid);

    if (old_fid == new_fid) {
        return;
    }

    mac_itr = PSEC_mac_itr_get_first();
    while (mac_itr != psec_mac_map.end()) {
        // Keep a pointer to the next, because it may be that we delete the
        // current below.
        mac_itr_next = mac_itr;
        mac_itr_next++;

        if (!mac_itr->second.in_mac_module || mac_itr->first.vlan != vid) {
            goto next;
        }

        (void)PSEC_mac_itr_to_str(mac_itr, buf);

        if (PSEC_ifindex_from_ifindex(mac_itr->first.ifindex, &psec_ifindex, __LINE__) != VTSS_RC_OK) {
            goto next;
        }

        // See if there's any such MAC address on the new FID. If so, don't
        // create a new entry for the one we are currently processing, but keep
        // that other entry.
        tmp_mac_itr = PSEC_mac_itr_get_from_fid_mac(new_fid, &mac_itr->first.mac);

        if (mac_itr == tmp_mac_itr) {
            // This is handled by the PSEC_LIMIT module. Nothing to do.
            T_I("%s: Skipping (handled by PSEC_LIMIT)", buf);
            goto next;
        }

        if (tmp_mac_itr != psec_mac_map.end()) {
            if (PSEC_interface_status_get(psec_ifindex.ifindex, &port_state, __LINE__) != VTSS_RC_OK) {
                goto next;
            }

            T_I("%s: Silently deleting, because another MAC (%s) is found on the new SVL (%u)", buf, PSEC_mac_itr_to_str(tmp_mac_itr, buf2), new_fid);
            PSEC_mac_del(&psec_ifindex, &port_state, mac_itr, PSEC_DEL_REASON_SVL_CHANGE, VTSS_APPL_PSEC_USER_LAST, VTSS_APPL_PSEC_USER_ADMIN);

            // Gotta check whether this gave rise to a new CPU-copy on the port.
            PSEC_sec_learn_cpu_copy_check(&psec_ifindex, &port_state, PSEC_LEARN_CPU_REASON_OTHER, __LINE__);

            // Save any changes back to the state.
            (void)PSEC_interface_status_set(psec_ifindex.ifindex, &port_state, __LINE__);
        } else {
            // We bypass the PSEC_mac_module_chg() and PSEC_mac_module_del()
            // functions, because otherwise we would also change the IP filter
            // in the kernel from one VID to the same VID, which is a waste of time.
            T_IG(TRACE_GRP_MAC_MODULE, "%s: Replacing MAC entry from FID = %u to %u", buf, old_fid, new_fid);

            if (mac_itr->second.status.mac_type != VTSS_APPL_PSEC_MAC_TYPE_DYNAMIC) {
                // The PSEC_LIMIT module is supposed to have handled all the
                // sticky and static entries already.
                T_E("%s: Internal error: Why hasn't PSEC_LIMIT taken care of this non-dynamic entry?", buf);
            }

            // Remove the old entry from the MAC table
            PSEC_mesa_mac_entry_init(entry, mac_itr, psec_ifindex.port);
            if ((rc = mesa_mac_table_del(NULL, &entry.vid_mac)) != VTSS_RC_OK) {
                T_EG(TRACE_GRP_MAC_MODULE, "%s: mesa_mac_table_del() failed (rc = %s)", buf, error_txt(rc));
            }

            // Add the new entry on the new FID
            entry.vid_mac.vid = new_fid;
            mac_itr->second.fid = new_fid;
            if ((rc = mesa_mac_table_add(NULL, &entry)) != VTSS_RC_OK) {
                mac_itr->second.in_mac_module = FALSE;
                T_EG(TRACE_GRP_MAC_MODULE, "%s: mesa_mac_table_add() failed (rc = %s)", buf, error_txt(rc));
            }
        }

next:
        mac_itr = mac_itr_next;
    }
}

/******************************************************************************/
// psec_mgmt_register_callbacks()
/******************************************************************************/
mesa_rc psec_mgmt_register_callbacks(vtss_appl_psec_user_t user, psec_on_mac_add_callback_f *on_mac_add_callback_func, psec_on_mac_del_callback_f *on_mac_del_callback_func)
{
    if (user >= VTSS_APPL_PSEC_USER_LAST) {
        return VTSS_APPL_PSEC_RC_INV_USER;
    }

    // Allow this function on any switch (primary or secondary switch), and
    // allow both NULL and non-NULL callbacks.
    PSEC_CRIT_ENTER();
    PSEC_state.on_mac_add_callbacks[user] = on_mac_add_callback_func;
    PSEC_state.on_mac_del_callbacks[user] = on_mac_del_callback_func;
    PSEC_CRIT_EXIT();
    return VTSS_RC_OK;
}

/****************************************************************************/
/*                                                                          */
/*  PUBLIC FUNCTIONS                                                        */
/*  These functions are meant for management use.                           */
/*                                                                          */
/****************************************************************************/

//******************************************************************************
// vtss_appl_psec_capabilities_get()
//******************************************************************************
mesa_rc vtss_appl_psec_capabilities_get(vtss_appl_psec_capabilities_t *const cap)
{
    if (!cap) {
        return VTSS_APPL_PSEC_RC_INV_PARAM;
    }

    cap->users = 0;
#if defined(VTSS_SW_OPTION_PSEC_LIMIT)
    cap->users |= VTSS_BIT(VTSS_APPL_PSEC_USER_ADMIN);
#endif

#ifdef VTSS_SW_OPTION_DOT1X
    cap->users |= VTSS_BIT(VTSS_APPL_PSEC_USER_DOT1X);
#endif

#ifdef DHCP_SNOOPING_MAC_VERI_SUPPORT
    cap->users |= VTSS_BIT(VTSS_APPL_PSEC_USER_DHCP_SNOOPING);
#endif

#ifdef VTSS_SW_OPTION_VOICE_VLAN
    cap->users |= VTSS_BIT(VTSS_APPL_PSEC_USER_VOICE_VLAN);
#endif

    cap->pool_size         = PSEC_MAC_ADDR_ENTRY_CNT;
    cap->limit_min         = PSEC_LIMIT_MIN;
    cap->limit_max         = PSEC_LIMIT_MAX;
    cap->violate_limit_min = PSEC_VIOLATE_LIMIT_MIN;
    cap->violate_limit_max = PSEC_VIOLATE_LIMIT_MAX;
    cap->age_time_min      = PSEC_AGE_TIME_MIN;
    cap->age_time_max      = PSEC_AGE_TIME_MAX;
    cap->hold_time_min     = PSEC_HOLD_TIME_MIN;
    cap->hold_time_max     = PSEC_HOLD_TIME_MAX;

    return VTSS_RC_OK;
}

//******************************************************************************
// vtss_appl_psec_global_status_get()
//******************************************************************************
mesa_rc vtss_appl_psec_global_status_get(vtss_appl_psec_global_status_t *const global_status)
{
    if (!global_status) {
        return VTSS_APPL_PSEC_RC_INV_PARAM;
    }

    PSEC_CRIT_ENTER();
    global_status->total_mac_cnt = PSEC_MAC_ADDR_ENTRY_CNT;
    global_status->cur_mac_cnt   = PSEC_state.macs_left;
    PSEC_CRIT_EXIT();

    return VTSS_RC_OK;
}

//******************************************************************************
// vtss_appl_psec_global_notification_status_get()
//******************************************************************************
mesa_rc vtss_appl_psec_global_notification_status_get(vtss_appl_psec_global_notification_status_t *const global_notif_status)
{
    if (!global_notif_status) {
        return VTSS_APPL_PSEC_RC_INV_PARAM;
    }

    // No need to lock scope, because the .get() function is guaranteed to be atomic.
    return psec_global_notification_status.get(global_notif_status);
}

//******************************************************************************
// vtss_appl_psec_interface_status_get()
//******************************************************************************
mesa_rc vtss_appl_psec_interface_status_get(vtss_ifindex_t ifindex, vtss_appl_psec_interface_status_t *const port_status)
{
    psec_semi_public_interface_status_t semi_public_port_state;

    if (!port_status) {
        return VTSS_APPL_PSEC_RC_INV_PARAM;
    }

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_PSEC_RC_MUST_BE_PRIMARY_SWITCH;
    }

    // No need to lock scope, because the .get() function is guaranteed to be atomic.
    VTSS_RC(psec_semi_public_interface_status.get(ifindex, &semi_public_port_state));

    *port_status = semi_public_port_state.status;

    return VTSS_RC_OK;
}

//******************************************************************************
// vtss_appl_psec_interface_notification_status_get()
//******************************************************************************
mesa_rc vtss_appl_psec_interface_notification_status_get(vtss_ifindex_t ifindex, vtss_appl_psec_interface_notification_status_t *const notification_status)
{
    if (!notification_status) {
        return VTSS_APPL_PSEC_RC_INV_PARAM;
    }

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_PSEC_RC_MUST_BE_PRIMARY_SWITCH;
    }

    // No need to lock scope, because the .get() function is guaranteed to be atomic.
    return psec_interface_notification_status.get(ifindex, notification_status);
}

/******************************************************************************/
// vtss_appl_psec_interface_status_mac_get_all()
/******************************************************************************/
mesa_rc vtss_appl_psec_interface_status_mac_get_all(vtss_appl_psec_mac_status_map_t &mac_status)
{
    psec_mac_itr_t mac_itr;

    PSEC_LOCK_SCOPE();

    for (mac_itr = PSEC_mac_itr_get_first(); mac_itr != psec_mac_map.end(); ++mac_itr) {
        // Filter out those that are not present in the H/W MAC table.
        if (!mac_itr->second.in_mac_module) {
            continue;
        }

        mac_status.insert(vtss::Pair<vtss_appl_psec_mac_map_key_t, vtss_appl_psec_mac_status_t>(mac_itr->first, mac_itr->second.status));
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psec_interface_status_mac_get()
/******************************************************************************/
mesa_rc vtss_appl_psec_interface_status_mac_get(vtss_ifindex_t ifindex, mesa_vid_t vid, mesa_mac_t mac, vtss_appl_psec_mac_status_t *mac_status)
{
    vtss_appl_psec_user_t        user;
    psec_mac_itr_t               mac_itr;
    vtss_appl_psec_mac_map_key_t key;

    if (!mac_status) {
        return VTSS_APPL_PSEC_RC_INV_PARAM;
    }

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_PSEC_RC_MUST_BE_PRIMARY_SWITCH;
    }

    memset(&key, 0, sizeof(key));
    key.ifindex = ifindex;
    key.vlan    = vid;
    key.mac     = mac;

    {
        PSEC_LOCK_SCOPE();

        if ((mac_itr = PSEC_mac_itr_get(&key)) == psec_mac_map.end()) {
            // Not found
            return VTSS_APPL_PSEC_RC_MAC_VID_NOT_FOUND;
        }

        // Make sure only to return MAC addresses that are actually in the MAC table.
        if (!mac_itr->second.in_mac_module) {
            return VTSS_RC_ERROR;
        }

        *mac_status = mac_itr->second.status;

        // A few fields need to be updated here, because they are not updated by this module otherwise.
        (void)misc_time2str_r(msg_abstime_get(VTSS_ISID_LOCAL, mac_itr->second.creation_time_secs), mac_status->creation_time);
        (void)misc_time2str_r(msg_abstime_get(VTSS_ISID_LOCAL, mac_itr->second.changed_time_secs),  mac_status->changed_time);

        mac_status->users_forward      = 0;
        mac_status->users_block        = 0;
        mac_status->users_keep_blocked = 0;
        for (user = (vtss_appl_psec_user_t)0; user < VTSS_APPL_PSEC_USER_LAST; user++) {
            psec_add_method_t method   = mac_itr->second.forward_decision[user];
            u32               user_bit = VTSS_BIT(user);
            mac_status->users_forward      |= (method == PSEC_ADD_METHOD_FORWARD      ? user_bit : 0);
            mac_status->users_block        |= (method == PSEC_ADD_METHOD_BLOCK        ? user_bit : 0);
            mac_status->users_keep_blocked |= (method == PSEC_ADD_METHOD_KEEP_BLOCKED ? user_bit : 0);
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psec_interface_status_mac_itr()
// Could have used vtss::iteratorComposeN, but that would cause billions and
// billions of calls into vtss_appl_psec_interface_status_mac_get(), because
// all ifindices, all VIDs, and all possible MAC addresses would have to be
// tried out - most of them returning false.
/******************************************************************************/
mesa_rc vtss_appl_psec_interface_status_mac_itr(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex,
                                                const mesa_vid_t     *prev_vid,     mesa_vid_t     *next_vid,
                                                const mesa_mac_t     *prev_mac,     mesa_mac_t     *next_mac)
{
    char           mac_str[18];
    psec_mac_itr_t mac_itr;

    T_I("prev_ifindex = %u, prev_vid = %u, prev_mac = %s", prev_ifindex ? VTSS_IFINDEX_PRINTF_ARG(*prev_ifindex) : -1, prev_vid ? *prev_vid : -1, prev_mac ? misc_mac_txt(prev_mac->addr, mac_str) : "(NULL)");

    if (!next_ifindex || !next_vid || !next_mac) {
        T_E("Invalid next pointer");
        return VTSS_RC_ERROR;
    }

    {
        PSEC_LOCK_SCOPE();

        // If prev_ifindex is NULL, then it's guaranteed that so are prev_vid and prev_mac
        if (!prev_ifindex) {
            T_D("Here");

            mac_itr = PSEC_mac_itr_find_current_or_next_in_mac_module(PSEC_mac_itr_get_first());
            goto do_exit;
        }

        // Here, we have a valid prev_ifindex. Start with that one
        if ((mac_itr = PSEC_mac_itr_get_first_from_ifindex(*prev_ifindex)) == psec_mac_map.end()) {
            T_D("Here");
            mac_itr = PSEC_mac_itr_get_next_from_ifindex(*prev_ifindex);
        }

        if ((mac_itr = PSEC_mac_itr_find_current_or_next_in_mac_module(mac_itr)) == psec_mac_map.end()) {
            T_D("Here");
            goto do_exit;
        }

        if (mac_itr->first.ifindex != *prev_ifindex || !prev_vid) {
            // This one is on the next ifindex or the caller hasn't got a VID preference.
            // Anyhow, exit with success.
            T_D("Here");
            goto do_exit;
        }

        // Here, the user has a VID preference.
        // Search for the first MAC on this ifindex with a VID == *prev_vid
        while (mac_itr != psec_mac_map.end() && mac_itr->first.ifindex == *prev_ifindex && mac_itr->first.vlan < *prev_vid) {
            T_D("Here");
            mac_itr = PSEC_mac_itr_find_current_or_next_in_mac_module(++mac_itr);
        }

        if (mac_itr == psec_mac_map.end()) {
            T_D("Here");
            goto do_exit;
        }

        if (mac_itr->first.ifindex != *prev_ifindex || mac_itr->first.vlan != *prev_vid || !prev_mac) {
            // This one is on the next ifindex or the next vid or the caller hasn't got a MAC preference.
            T_D("Here");
            goto do_exit;
        }

        // Here, the user has a MAC preference.
        // Search for the first MAC with a MAC address > the *prev_mac
        while (mac_itr != psec_mac_map.end() && mac_itr->first.ifindex == *prev_ifindex && mac_itr->first.vlan == *prev_vid && memcmp(&mac_itr->first.mac, prev_mac, sizeof(mac_itr->first.mac)) <= 0) {
            mac_itr = PSEC_mac_itr_find_current_or_next_in_mac_module(++mac_itr);
        }

do_exit:
        if (mac_itr != psec_mac_map.end()) {
            *next_ifindex = mac_itr->first.ifindex;
            *next_vid     = mac_itr->first.vlan;
            *next_mac     = mac_itr->first.mac;
            T_I("Exit. Found one: next_ifindex = %u, next_vid = %u, next_mac = %s", VTSS_IFINDEX_PRINTF_ARG(*next_ifindex), *next_vid, misc_mac_txt(next_mac->addr, mac_str));
            return VTSS_RC_OK;
        }
    }

    T_I("Exit. No next");
    return VTSS_RC_ERROR;
}

/******************************************************************************/
// vtss_appl_psec_user_info_get()
/******************************************************************************/
mesa_rc vtss_appl_psec_user_info_get(vtss_appl_psec_user_t user, vtss_appl_psec_user_info_t *const info)
{
    if (!info) {
        return VTSS_APPL_PSEC_RC_INV_PARAM;
    }

    if (user >= VTSS_APPL_PSEC_USER_LAST) {
        return VTSS_APPL_PSEC_RC_INV_USER;
    }

    info->name[sizeof(info->name) - 1] = '\0';
    snprintf(info->name, sizeof(info->name) - 1, "%s", PSEC_user_name(user));

    info->abbr = PSEC_user_abbr(user);

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psec_global_control_mac_clear()
// The public pendant to psec_mgmt_mac_del()
/******************************************************************************/
mesa_rc vtss_appl_psec_global_control_mac_clear(const vtss_appl_psec_global_control_mac_clear_t *info)
{
    psec_ifindex_t          psec_ifindex;
    vtss_ifindex_t          prev_ifindex, ifindex;
    psec_interface_status_t port_state;
    psec_mac_itr_t          mac_itr, temp_mac_itr;
    BOOL                    first = TRUE, done = FALSE;

    if (!info) {
        return VTSS_APPL_PSEC_RC_INV_PARAM;
    }

    if (!msg_switch_is_primary()) {
        return VTSS_APPL_PSEC_RC_MUST_BE_PRIMARY_SWITCH;
    }

    T_I("specific_ifindex = %d, specific_vlan = %d, specific_mac = %d", info->specific_ifindex, info->specific_vlan, info->specific_mac);

    if (info->specific_vlan) {
        if (info->vlan < VTSS_APPL_VLAN_ID_MAX || info->vlan > VTSS_APPL_VLAN_ID_MAX) {
            return VTSS_APPL_PSEC_RC_INV_VLAN;
        }
    }

    {
        PSEC_LOCK_SCOPE();

        while (!done) {
            BOOL hit = FALSE;

            if (info->specific_ifindex) {
                // Only looking at one single ifindex, so we're done when done with this iteration.
                ifindex = info->ifindex;
                done = TRUE;
            } else {
                // Loop through all ports
                if (vtss_ifindex_getnext_port_exist(first ? NULL : &prev_ifindex, &ifindex) != VTSS_RC_OK) {
                    // Done
                    break;
                }

                first = FALSE;
                prev_ifindex = ifindex;
            }

            // Last parameter tells PSEC_ifindex_from_ifindex() not to throw a trace error in case the user-specified ifindex is invalid
            VTSS_RC(PSEC_ifindex_from_ifindex(ifindex, &psec_ifindex, __LINE__, !info->specific_ifindex));

            VTSS_RC(PSEC_interface_status_get(psec_ifindex.ifindex, &port_state, __LINE__));

            if ((mac_itr = PSEC_mac_itr_get_first_from_ifindex(psec_ifindex.ifindex)) == psec_mac_map.end()) {
                // No MAC addresses learned on this interface.
                continue;
            }

            if (info->specific_vlan || info->specific_mac) {
                while (mac_itr != psec_mac_map.end() && mac_itr->first.ifindex == psec_ifindex.ifindex) {
                    temp_mac_itr = mac_itr;
                    temp_mac_itr++;

                    if (info->specific_vlan && info->vlan != mac_itr->second.status.vid_mac.vid) {
                        goto next;
                    }

                    if (info->specific_mac && memcmp(&info->mac, &mac_itr->second.status.vid_mac.mac, sizeof(info->mac)) != 0) {
                        goto next;
                    }

                    if (mac_itr->second.status.mac_type != VTSS_APPL_PSEC_MAC_TYPE_DYNAMIC) {
                        // Can only remove dynamic entries with this function
                        goto next;
                    }

                    // Hit.
                    PSEC_mac_del(&psec_ifindex, &port_state, mac_itr, PSEC_DEL_REASON_USER_DELETED, VTSS_APPL_PSEC_USER_LAST, VTSS_APPL_PSEC_USER_LAST);
                    hit = TRUE;

next:
                    mac_itr = temp_mac_itr;
                }
            } else {
                // User isn't looking for a specific VLAN or a specific MAC.
                // He just wanna remove everything. However, he can only remove dynamic entries.
                PSEC_mac_del_all(&psec_ifindex, &port_state, PSEC_DEL_REASON_USER_DELETED, VTSS_APPL_PSEC_USER_LAST, FALSE);
                hit = TRUE;
            }

            if (hit) {
                // This may have given rise to reopening the port for CPU-copy traffic.
                PSEC_sec_learn_cpu_copy_check(&psec_ifindex, &port_state, PSEC_LEARN_CPU_REASON_OTHER, __LINE__);

                // Save back the new state
                VTSS_RC(PSEC_interface_status_set(psec_ifindex.ifindex, &port_state, __LINE__));
            }
        }
    }

    return VTSS_RC_OK;
}

/****************************************************************************/
// psec_error_txt()
/****************************************************************************/
const char *psec_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_APPL_PSEC_RC_INV_USER:
        return "Invalid user";

    case VTSS_APPL_PSEC_RC_MUST_BE_PRIMARY_SWITCH:
        return "Operation only valid on the primary switch";

    case VTSS_APPL_PSEC_RC_INV_ISID:
        return "Invalid Switch ID";

    case VTSS_APPL_PSEC_RC_INV_PORT:
        return "Invalid port number";

    case VTSS_APPL_PSEC_RC_INV_IFINDEX:
        return "Unable to decompose ifindex";

    case VTSS_APPL_PSEC_RC_IFINDEX_NOT_REPRESENTING_A_PORT:
        return "Ifindex not representing a port";

    case VTSS_APPL_PSEC_RC_INV_VLAN:
        return "Invalid VLAN";

    case VTSS_APPL_PSEC_RC_INV_AGING_PERIOD:
        // Not nice to use specific values in this string, but
        // much easier than constructing a constant string dynamically.
        return "Aging period is out of bounds (0 or [" vtss_xstr(PSEC_AGE_TIME_MIN) "; " vtss_xstr(PSEC_AGE_TIME_MAX) "])";

    case VTSS_APPL_PSEC_RC_INV_HOLD_TIME:
        // Not nice to use specific values in this string, but
        // much easier than constructing a constant string dynamically.
        return "Hold time is out of bounds ([" vtss_xstr(PSEC_HOLD_TIME_MIN) "; " vtss_xstr(PSEC_HOLD_TIME_MAX) "])";

    case VTSS_APPL_PSEC_RC_MAC_VID_NOT_FOUND:
        return "The <MAC, VLAN> was not found on the specified interface";

    case VTSS_APPL_PSEC_RC_MAC_VID_ALREADY_FOUND:
        return "The <MAC, VLAN> is already installed on another interface";

    case VTSS_APPL_PSEC_RC_MAC_VID_ALREADY_FOUND_ON_SVL:
        return "The <MAC, VLAN> is already installed on another VLAN mapping to the same shared VLAN";

    case VTSS_APPL_PSEC_RC_INV_USER_MODE:
        return "The PSEC user is not allowed to call this function";

    case VTSS_APPL_PSEC_RC_SWITCH_IS_DOWN:
        return "The selected switch doesn't exist";

    case VTSS_APPL_PSEC_RC_LINK_IS_DOWN:
        return "The selected port's link is down";

    case VTSS_APPL_PSEC_RC_MAC_POOL_DEPLETED:
        return "Port Security-controlled MAC pool depleted";

    case VTSS_APPL_PSEC_RC_PORT_IS_SHUT_DOWN:
        return "The port has been shut down";

    case VTSS_APPL_PSEC_RC_LIMIT_IS_REACHED:
        return "The limit is reached - no more MAC addresses can be added to this port";

    case VTSS_APPL_PSEC_RC_NO_USERS_ENABLED:
        return "No users are enabled on the port";

    case VTSS_APPL_PSEC_RC_STP_MSTI_DISCARDING:
        return "The port STP MSTI state is discarding";

    case VTSS_APPL_PSEC_RC_INV_PARAM:
        return "Invalid parameter supplied to function";

    case VTSS_APPL_PSEC_RC_END_OF_LIST:
        // Not a real error. Simply marks the end of a list in iterator functions.
        return "End of list";

    case VTSS_APPL_PSEC_RC_OUT_OF_MEMORY:
        return "Out of memory";

    case VTSS_APPL_PSEC_RC_MAC_NOT_UNICAST:
        return "MAC address is not a unicast MAC address";

    case VTSS_APPL_PSEC_RC_INV_RATE_LIMITER_FILL_LEVEL:
        return "Maximum fill-level must be greater than the minimum";

    case VTSS_APPL_PSEC_RC_INV_RATE_LIMITER_RATE:
        return "The rate-limiter rate must be greater than 0";

    case VTSS_APPL_PSEC_RC_INTERNAL_ERROR:
        return "An internal error occurred";

    default:
        return "PSEC: Unknown error code";
    }
}

/******************************************************************************/
// psec_del_reason_to_str()
/******************************************************************************/
const char *psec_del_reason_to_str(psec_del_reason_t reason)
{
    switch (reason) {
    case PSEC_DEL_REASON_HW_ADD_FAILED:
        return "MAC Table add failed (H/W)";
    case PSEC_DEL_REASON_SW_ADD_FAILED:
        return "MAC Table add failed (S/W)";
    case PSEC_DEL_REASON_PORT_LINK_DOWN:
        return "The port link went down";
    case PSEC_DEL_REASON_STATION_MOVED:
        return "The MAC was suddenly seen on another port";
    case PSEC_DEL_REASON_AGED_OUT:
        return "The entry aged out";
    case PSEC_DEL_REASON_HOLD_TIME_EXPIRED:
        return "The hold time expired";
    case PSEC_DEL_REASON_USER_DELETED:
        return "The entry was deleted by another module";
    case PSEC_DEL_REASON_PORT_SHUT_DOWN:
        return "Shut down by Limit Control module";
    case PSEC_DEL_REASON_NO_MORE_USERS:
        return "No more users";
    case PSEC_DEL_REASON_PORT_STP_MSTI_DISCARDING:
        return "The port STP MSTI state is discarding";
    default:
        return "Unknown";
    }
}

/******************************************************************************/
// psec_add_method_to_str()
/******************************************************************************/
const char *psec_add_method_to_str(psec_add_method_t add_method)
{
    switch (add_method) {
    case PSEC_ADD_METHOD_FORWARD:
        return "Forward";
    case PSEC_ADD_METHOD_BLOCK:
        return "Block with timeout";
    case PSEC_ADD_METHOD_KEEP_BLOCKED:
        return "Keep blocked";
    default:
        return "Unknown";
    }
}

#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_psec_json_init();
#endif

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void psec_mib_init();
#endif

extern "C" int psec_icli_cmd_register();

/******************************************************************************/
// psec_init()
// Initialize Port Security Module
/******************************************************************************/
mesa_rc psec_init(vtss_init_data_t *data)
{
    vtss_isid_t                                 isid = data->isid;
    mesa_port_no_t                              port;
    psec_ifindex_t                              psec_ifindex;
    psec_interface_status_t                     port_state;
    vtss_appl_psec_global_notification_status_t global_notif_status;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        // Initialize message buffer(s)
        // We need one per port to avoid stalling user modules. The buffers aren't that big (32 bytes each at the time of writing).
        PSEC_msg_buf_pool = msg_buf_pool_create(VTSS_MODULE_ID_PSEC, "Request", fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT), sizeof(psec_msg_t));

        // Create the needed entries in psec_semi_public_interface_status and
        // psec_interface_notification_status.
        // This code cannot use port_iter_t, because it's are not allowed until
        // we're the primary switch if we wish to iterate over all switches.
        memset(&port_state, 0, sizeof(port_state));

        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            for (port = 0; port < port_count_max(); port++) {
                if (PSEC_ifindex_from_port(isid, port, &psec_ifindex, __LINE__) != VTSS_RC_OK) {
                    continue;
                }

                (void)PSEC_interface_status_set(psec_ifindex.ifindex, &port_state, __LINE__);
            }
        }

        memset(&global_notif_status, 0, sizeof(global_notif_status));
        psec_global_notification_status.set(&global_notif_status);

        // Compile and run-time checks

        // Avoid Lint warning "Constant value Boolean". This is intended to be a compile time check
        /*lint -e{506} */
        if (VTSS_APPL_PSEC_USER_LAST > (vtss_appl_psec_user_t)32) {
            T_E("This module supports at most 32 users due to vtss_appl_psec_interface_status_t::users");
        }

#if PSEC_MAC_ADDR_ENTRY_CNT <= 0
#error "Invalid PSEC_MAC_ADDR_ENTRY_CNT"
#endif

        PSEC_state.macs_left = PSEC_MAC_ADDR_ENTRY_CNT;

        // Initialize mutex.
        critd_init(&crit_psec, "psec", VTSS_MODULE_ID_PSEC, CRITD_TYPE_MUTEX);

#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_psec_json_init();
#endif

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        psec_mib_init();
#endif
        psec_icli_cmd_register();
        break;

    case INIT_CMD_START:
        vtss_flag_init(&PSEC_thread_wait_flag);

        // Initialize the thread
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           PSEC_thread,
                           0,
                           "Port Security",
                           nullptr,
                           0,
                           &PSEC_thread_handle,
                           &PSEC_thread_state);

        PSEC_msg_rx_init();
        PSEC_frame_rx_init();

        // Register for port link-state change events
        (void)port_change_register(VTSS_MODULE_ID_PSEC, PSEC_link_state_change_callback);














#if defined(VTSS_SW_OPTION_MSTP)
        // Register for port STP MSTI state change events
        (void)l2_stp_msti_state_change_register(PSEC_stp_msti_state_change_callback);
#endif /* VTSS_SW_OPTION_MSTP */
        break;

    case INIT_CMD_CONF_DEF:
        // We don't have any configuration.
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        // Nothing to do
        break;

    case INIT_CMD_ICFG_LOADING_POST: {
        psec_rate_limit_conf_t rate_limit_conf;

        // Send the whole switch configuration to the new switch.
        // It is assumed that this module is already told the whole story about
        // all user modules' enabledness. If not, it will result in many, many
        // small messages sent in the psec_mgmt_port_conf_set() function.
        PSEC_CRIT_ENTER();
        PSEC_switch_exists[isid - VTSS_ISID_START] = TRUE;
        PSEC_msg_tx_switch_conf(isid);

        // About race condition concerns: One could be concerned that this
        // module won't get the INIT_CMD_ICFG_LOADING_POST event until the port
        // module has sent link-up events for the switch.
        // This may occur if this module comes after the port module in the
        // array of modules. The good thing is that we (in
        // PSEC_link_state_change_callback()) react and cache the link state
        // even before this piece of code is called. This means that the
        // link-state may already be updated when we get here, so we don't have
        // to ask the port module for its link-state here.

        // Check if this event gave rise to changing the secure learning on one or more ports on the new switch.
        // First we create a psec_ifindex. The only important field is psec_ifindex.isid. All other fields of psec_ifindex
        // are not used by PSEC_sec_learn_cpu_copy_check() when using PSEC_LEARN_CPU_REASON_SWITCH_UP.
        if (PSEC_ifindex_from_port(isid, 0, &psec_ifindex, __LINE__) == VTSS_RC_OK) {
            // Then we call the function with the seconds argument (port_state) set to NULL. This
            // tells the function that it - itself - must get and set all port_state entries it may wanna change
            PSEC_sec_learn_cpu_copy_check(&psec_ifindex, NULL, PSEC_LEARN_CPU_REASON_SWITCH_UP, __LINE__);
        }

        PSEC_CRIT_EXIT();

        // Also send the configured rate-limit to the new switch (outside the crit sect).
        psec_rate_limit_conf_get(&rate_limit_conf);
        psec_msg_tx_rate_limit_conf(isid, &rate_limit_conf);
        break;
    }

    default:
        break;
    }

    return VTSS_RC_OK;
}

