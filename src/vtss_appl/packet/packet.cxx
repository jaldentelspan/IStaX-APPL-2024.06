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

#include "vtss_os_wrapper.h"
#include "packet_api.h"
#include "packet.h"
#include "vtss_fifo_api.h"    /* Contains a number of functions for managing a FIFO whose items are of fixed size (sizeof(void *)) */
#include "vtss_fifo_cp_api.h" /* Contains a number of functions for managing a FIFO whose items are of variable size */
#include "vlan_api.h"
#include "misc_api.h"         /* For iport2uport()            */
#include "msg_api.h"          /* For msg_max_user_prio()      */
#include "vtss_api_if_api.h"  /* For vtss_api_if_chip_count() */
#include "vtss_timer_api.h"

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/mman.h>         /* For mmap() */
#include <poll.h>             /* For poll() */
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <errno.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/genetlink.h>
#include <linux/filter.h>
#include <sys/stat.h>
#include "vtss_netlink.hxx"

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_PACKET

#define PACKET_INTERNAL_FLAGS_AFI_CANCEL 0x1

// Trace registration
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "packet", "Packet module"
};

#if !defined(PACKET_DEFAULT_TRACE_LVL)
#define PACKET_DEFAULT_TRACE_LVL VTSS_TRACE_LVL_ERROR
#endif /* !defined(PACKET_DEFAULT_TRACE_LVL) */

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        PACKET_DEFAULT_TRACE_LVL,
        VTSS_TRACE_FLAGS_USEC
    },
    [TRACE_GRP_RX] = {
        "rx",
        "Dump of received packets (info => hdr, noise => data).",
        PACKET_DEFAULT_TRACE_LVL,
        VTSS_TRACE_FLAGS_USEC
    },
    [TRACE_GRP_TX] = {
        "tx",
        "Dump of transmitted packets (lvl = noise).",
        PACKET_DEFAULT_TRACE_LVL,
        VTSS_TRACE_FLAGS_USEC
    },
    [TRACE_GRP_NETLINK] = {
        "netlink",
        "Netlink calls",
        PACKET_DEFAULT_TRACE_LVL,
        VTSS_TRACE_FLAGS_USEC
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

// When running Linux on the internal CPU, we can ask
// the uFDMA running in the kernel to output trace, so
// we register its own trace layers and groups and use
// netlink to transfer the config to the kernel.

// Unfortunately, we have to replicate a few constants in user-space,
// because we don't have the kernel header file available for these.
#define UFDMA_TRACE_LAYER_AIL   0
#define UFDMA_TRACE_LAYER_CIL   1

#define UFDMA_TRACE_GRP_DEFAULT 0
#define UFDMA_TRACE_GRP_RX      1
#define UFDMA_TRACE_GRP_TX      2

#define UFDMA_TRACE_LEVEL_NONE  0
#define UFDMA_TRACE_LEVEL_ERROR 1
#define UFDMA_TRACE_LEVEL_INFO  2
#define UFDMA_TRACE_LEVEL_DEBUG 3

// Trace registration
static vtss_trace_reg_t ufdma_ail_trace_reg = {
    VTSS_MODULE_ID_UFDMA_AIL, "ufdma_ail", "uFDMA - Application Interface Layer"
};

static vtss_trace_reg_t ufdma_cil_trace_reg = {
    VTSS_MODULE_ID_UFDMA_CIL, "ufdma_cil", "uFDMA - Chip Interface Layer"
};

static vtss_trace_grp_t ufdma_ail_trace_grps[] = {
    [UFDMA_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [UFDMA_TRACE_GRP_RX] = {
        "rx",
        "Trace in Rx direction",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [UFDMA_TRACE_GRP_TX] = {
        "tx",
        "Trace in Tx direction",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
};

VTSS_TRACE_REGISTER(&ufdma_ail_trace_reg, ufdma_ail_trace_grps);

// The UFDMA CIL layer trace initialization is deferred to packet_init()
static vtss_trace_grp_t ufdma_cil_trace_grps[ARRSZ(ufdma_ail_trace_grps)];

static          critd_t RX_filter_crit;
static          critd_t RX_dispatch_thread_crit;
static          critd_t CX_counter_crit;
static          u32     RX_ifh_size;
static          u32     RX_fcs_data;
static volatile u32     RX_mtu; // As configured in uFDMA
static          u32     TX_insert_tag;
static          u32     TX_masq_port;
static          u32     TX_ptp_action;
static          bool    CX_internal_cpu;
static          bool    CX_internal_mips;
static          u32     CX_port_cnt;
static          bool    TX_ifh_has_dst_port_mask;

// Macros for accessing semaphore functions
#define PACKET_RX_FILTER_CRIT_ENTER()                  critd_enter(        &RX_filter_crit,          __FILE__, __LINE__)
#define PACKET_RX_FILTER_CRIT_EXIT()                   critd_exit(         &RX_filter_crit,          __FILE__, __LINE__)
#define PACKET_RX_FILTER_CRIT_ASSERT_LOCKED()          critd_assert_locked(&RX_filter_crit,          __FILE__, __LINE__)
#define PACKET_RX_DISPATCH_THREAD_CRIT_ENTER()         critd_enter(        &RX_dispatch_thread_crit, __FILE__, __LINE__)
#define PACKET_RX_DISPATCH_THREAD_CRIT_EXIT()          critd_exit(         &RX_dispatch_thread_crit, __FILE__, __LINE__)
#define PACKET_RX_DISPATCH_THREAD_CRIT_ASSERT_LOCKED() critd_assert_locked(&RX_dispatch_thread_crit, __FILE__, __LINE__)
#define PACKET_CX_COUNTER_CRIT_ENTER()                 critd_enter(        &CX_counter_crit,         __FILE__, __LINE__)
#define PACKET_CX_COUNTER_CRIT_EXIT()                  critd_exit(         &CX_counter_crit,         __FILE__, __LINE__)
#define PACKET_CX_COUNTER_CRIT_ASSERT_LOCKED()         critd_assert_locked(&CX_counter_crit,         __FILE__, __LINE__)

#define FDMA_INTERRUPT CYGNUM_HAL_INTERRUPT_FDMA

// This macro must *not* evaluate to an empty macro, since it's expected to do some useful stuff.
#define PACKET_TX_CHECK(x) do {if (!(x)) {T_E("Assertion failed: " #x); return PACKET_RC_TX_CHECK;}} while (0)

/***************************************************/
// Common Static Data
/***************************************************/
static packet_port_counters_t   CX_port_counters;
static packet_module_counters_t CX_module_counters[VTSS_MODULE_ID_NONE + 1];
static BOOL                     CX_stack_trace_ena; // Let it be disabled by default
static int                      ifh_sock;
static uint64_t                 RX_xtr_qu_counters[8];

/***************************************************/
// Tx Static Data
/***************************************************/
static u32 TX_alloc_calls;
static u32 TX_free_calls;

// Parameters used in the throttling interface between user- and kernel space.
// Keep in sync with the Kernel-space definitions
enum {
    VTSS_PACKET_ATTR_NONE,                            /**< Must come first                                                           */
    VTSS_PACKET_ATTR_RX_THROTTLE_TICK_PERIOD_MSEC,    /**< Number of milliseconds between two throttle ticks, 0 to disable, max 1000 */
    VTSS_PACKET_ATTR_RX_THROTTLE_QU_CFG,              /**< Config for one queue consists of the following four parameters            */
    VTSS_PACKET_ATTR_RX_THROTTLE_QU_NUMBER,           /**< Must-be-present attribute identifying queue number                        */
    VTSS_PACKET_ATTR_RX_THROTTLE_FRM_LIMIT_PER_TICK,  /**< Max number of frames extracted between two ticks w/o suspension           */
    VTSS_PACKET_ATTR_RX_THROTTLE_BYTE_LIMIT_PER_TICK, /**< Max number of bytes extracted between two ticks w/o suspension            */
    VTSS_PACKET_ATTR_RX_THROTTLE_SUSPEND_TICK_CNT,    /**< Number of ticks to suspend when suspending                                */
    VTSS_PACKET_ATTR_TRACE_LAYER,                     /**< AIL (0) or CIL (1)                                                        */
    VTSS_PACKET_ATTR_TRACE_GROUP,                     /**< Default (0), Rx (1), Tx (2)                                               */
    VTSS_PACKET_ATTR_TRACE_LEVEL,                     /**< None (0), Error (1), Info (2), Debug (3)                                  */
    VTSS_PACKET_ATTR_RX_CFG_MTU,                      /**< Rx MTU [64; 16384]                                                        */
    VTSS_PACKET_ATTR_END,                             /**< Must come last                                                            */
};

#define VTSS_PACKET_ATTR_CNT (VTSS_PACKET_ATTR_END + 1)

// Functions working on one or more of the attributes above.
// Keep in sync with the Kernel-space definitions
enum {
    VTSS_PACKET_GENL_NOOP,                /** Must come first                                     */
    VTSS_PACKET_GENL_RX_THROTTLE_CFG_GET, /**< Get current throttle configuration from the kernel */
    VTSS_PACKET_GENL_RX_THROTTLE_CFG_SET, /**< Change throttle configuration in the kernel        */
    VTSS_PACKET_GENL_TRACE_CFG_SET,       /**< Set uFDMA trace level settings                     */
    VTSS_PACKET_GENL_RX_CFG_GET,          /**< Get Rx config                                      */
    VTSS_PACKET_GENL_RX_CFG_SET,          /**< Set Rx config                                      */
    VTSS_PACKET_GENL_STATI_CLEAR,         /**< Clear statistics                                   */
    // Add new operations here
};

#define INVOKE_FUNC(FUNC, ...)                                              \
    do {                                                                    \
        mesa_rc _rc_ = FUNC(__VA_ARGS__);                                   \
        if (_rc_ != VTSS_RC_OK) {                                           \
            T_EG(TRACE_GRP_NETLINK, #FUNC " failed. Error code: %u", _rc_); \
            return _rc_;                                                    \
        }                                                                   \
    } while (0)

#define CX_NETLINK_ATTR_ADD(T, TT, TTT) INVOKE_FUNC(vtss::appl::netlink::attr_add_##TT, &req.netlink_msg_hdr, req.max_size_bytes, T, TTT)

#define CX_NETLINK_ATTR_U32_GET(_attr_, _val_)                                        \
    do {                                                                              \
        if (RTA_PAYLOAD(_attr_) != 4) {                                               \
            T_EG(TRACE_GRP_NETLINK, "Expected 4, got %u bytes", RTA_PAYLOAD(_attr_)); \
            return;                                                                   \
        }                                                                             \
                                                                                      \
        _val_ = *(u32 *)RTA_DATA(_attr_);                                             \
    } while (0)

#define CX_NETLINK_ATTR_IDX_U32_GET(_attrs_, _idx_, _val_)                       \
    do {                                                                         \
        struct rtattr *_attr_ = _attrs_[_idx_];                                  \
        if (_attr_ == NULL) {                                                    \
            T_EG(TRACE_GRP_NETLINK, "Attribute with idx = %u not found", _idx_); \
            return;                                                              \
        }                                                                        \
                                                                                 \
        CX_NETLINK_ATTR_U32_GET(_attr_, _val_);                                  \
    } while (0)

/***************************************************/
// Rx Static Data
/***************************************************/
// This number should (could) be in accordance with the number of buffers allocated by the Linux kernel's uFDMA driver.
#define RX_BUF_CNT 1024

// RX thread variables
static vtss_handle_t RX_thread_handle;
static vtss_thread_t RX_thread_state;      // Contains space for the scheduler to hold the current thread state.
static vtss_sem_t    RX_packet_sem;        // Counting semaphore to avoid having the application receive more than a certain amount of frames without having them processed.
static vtss_mutex_t  RX_low_level_mutex;   // Only used for counting at the lowest level.
static u32           RX_outstanding;       // Counts number of frames the application currently is processing
static u32           RX_outstanding_max;   // Holds the maximum number of frames the application has had outstanding.
static u32           RX_total;             // Total number of frames received with recv()
static u32           RX_processing = true; // When true, received frames are forwarded to the listeners. Otherwise, they are discarded right away.

typedef struct {
    vtss_fifo_cp_t    fifo;                      // Contains the packet descriptors
    vtss_flag_t       flag;                      // Flag to signal when a new packet is ready in the FIFO.
    vtss_handle_t     thread_handle;             // Thread handle
    vtss_thread_t     thread_state;              // Contains space for the scheduler to hold the current thread state.
    char              thread_name[40];           // Name of thread
    u64               rx_bytes;                  // Bytes dispatched from this thread
    u32               rx_pkts;                   // Packets dispatched from this thread
    vtss_tick_count_t longest_rx_callback_ticks; // Number of ticks for slowest callback on this thread prio.
} packet_rx_dispatch_thread_state_t;

static packet_rx_dispatch_thread_state_t RX_dispatch_thread_states[VTSS_THREAD_PRIO_NA];

// This holds a list of received packets produced by RX_fdma_packet(),
// which is running in DSR context and consumed by RX_thread(),
// which is running in thread context.
static packet_rx_filter_item_t *RX_filter_list = NULL;
static BOOL                    RX_filter_list_changed; // Tells RX_filter_list_get() to update its cached list of packet Rx listeners.

// Rx counters. The per-rx-filter counters are held in the relevant list items.
// These two hold the counters for packets with no subscribers.
static u64 RX_bytes_no_subscribers;
static u32 RX_pkts_no_subscribers;

// Current value of S-Custom VLAN tag TPID
static mesa_etype_t RX_vlan_s_custom_tpid;

// Number of live filter lists. In a steady environment (no activity on RX_filter_list_changed),
// this should be 1. The RX_filter_lists_total counts the number of times the filter list has
// been updated during packet reception.
static u32 RX_filter_lists_alive;
static u32 RX_filter_lists_total;

// NPI encapsulation, EPID is set up using MESA_CAP_PACKET_IFH_EPID during initialization
static u8 npi_encap[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0x88, 0x80, 0x00, 0x00
};

#define VTSS_PROTO    0x8880            // IFH 'long' encapsulation
#define NPI_ENCAP_LEN sizeof(npi_encap) // DA = BC, SA, ETYPE = 88:80, 00:05

static struct sockaddr_ll npi_socket_address;

typedef struct {
    u32                     ref_cnt;
    u32                     item_cnt;
    packet_rx_filter_item_t *items;
} packet_rx_filter_list_t;

// Structure passed through Rx FIFOs to prioritized threads executed by RX_dispatch_thread().
typedef struct packet_dscr_s {
    // This is a pointer to a platform-specific structure, which is
    // only known to the caller of RX_thread_packet_handle().
    void *opaque;

    // Points to the first byte of the DMAC when the user-module gets called back.
    u8 *frm_ptr;

    // Decoded IFH
    mesa_packet_rx_info_t rx_info;

    // Length of frame excl. NPI_ENCAP, IFH, and FCS.
    u32 act_len;

    // Reference to the filter list currently being used for this frame.
    // This is ref-counted (protected by PACKET_RX_FILTER_CRIT_ENTER/EXIT())
    packet_rx_filter_list_t *filter_list;

    // Number of matches so far.
    u32 match_cnt;

    // Number of default-matches so far.
    u32 match_default_cnt;

    // This points to the filter in #filter_list that we're going to callback subscriber for.
    packet_rx_filter_item_t *current_filter;

    // This points to the next item in #filter_list to process.
    packet_rx_filter_item_t *next_filter;

    // This points to the next item in #filter_list when looking
    // for default-subscribers.
    packet_rx_filter_item_t *next_default_filter;

    // Only give this to sFlow subscribers when TRUE
    BOOL sflow_subscribers_only;

    // Outer tag must be stripped when TRUE, and must be pushed again
    // afterwards.
    BOOL strip_outer_tag;

    // For fast reference to the etype of this frame
    mesa_etype_t etype;

    // For fast reference to whether frame had an inner tag before an IP etype.
    BOOL outer_is_vlan_tag_and_inner_is_ip_any;

    // For fast reference to whether outer etype is an IP-type
    BOOL outer_is_ip_any;

} packet_dscr_t;

// Forward declaration (really needed here).
static BOOL RX_match_next(packet_dscr_t *packet_dscr);

/****************************************************************************/
/*                                                                          */
/*  NAMING CONVENTION FOR INTERNAL FUNCTIONS:                               */
/*    RX_<function_name> : Functions related to Rx (extraction).            */
/*    TX_<function_name> : Functions related to Tx (injection).             */
/*    CX_<function_name> : Functions related to both Rx and Tx (common).    */
/*    DBG_<function_name>: Functions related to debugging.                  */
/*                                                                          */
/*  NAMING CONVENTION FOR EXTERNAL (API) FUNCTIONS:                         */
/*    packet_rx_<function_name>: Functions related to Rx (extraction).      */
/*    packet_tx_<function_name>: Functions related to Tx (injection).       */
/*    packet_<function_name>   : Functions related to both Rx and Tx.       */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/*  MODULE INTERNAL FUNCTIONS                                               */
/*                                                                          */
/****************************************************************************/

/******************************************************************************/
// CX_npi_init()
/******************************************************************************/
static void CX_npi_init(void)
{
    struct ifreq ifr = {};
    const char   *ifname = VTSS_NPI_DEVICE;
    int          sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        T_E("Unable to UP %s", ifname);
        return;
    }

    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) >= 0) {
        strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
        ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);

        if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) >= 0) {
            T_D("%s is UP", ifname);
        } else {
            T_E("ioctl(%s, SIOCSIFFLAGS) failed: %s", ifname, strerror(errno));
        }
    } else {
        T_E("ioctl(%s, SIOCGIFFLAGS) failed: %s", ifname, strerror(errno));
    }

    close(sockfd);
}

#define CX_TRACE_FILE       "/switch/icfg/packet_trace"
#define CX_TRACE_FILE_LIMIT 2000000
static FILE *CX_trace_file;
static bool RX_trace_to_file;
static bool TX_trace_to_file;

/******************************************************************************/
// CX_dbg_packet_trace()
/******************************************************************************/
static void CX_trace_file_open_close(void)
{
    if (RX_trace_to_file || TX_trace_to_file) {
        if (!CX_trace_file) {
            CX_trace_file = fopen(CX_TRACE_FILE, "a");
        }
    } else {
        if (CX_trace_file) {
            fclose(CX_trace_file);
            CX_trace_file = NULL;
        }
    }
}

/******************************************************************************/
// CX_trace_to_file()
/******************************************************************************/
static void CX_trace_to_file(const u8 *frm_ptr, int size)
{
    struct stat buffer;
    int         status, i;

    status = stat(CX_TRACE_FILE, &buffer);
    if (status == 0 && buffer.st_size > CX_TRACE_FILE_LIMIT) {
        T_W("File %s exceed size %d. Disabling trace", CX_TRACE_FILE, CX_TRACE_FILE_LIMIT);
        RX_trace_to_file = false;
        TX_trace_to_file = false;
        CX_trace_file_open_close();
        return;
    }

    if (CX_trace_file) {
        for (i = 0; i < size; ++i) {
            fprintf(CX_trace_file, "%02X ", frm_ptr[i]);
        }

        fprintf(CX_trace_file, "\n");
    }
}

/******************************************************************************/
// packet_debug_rx_packet_trace()
/******************************************************************************/
void packet_debug_rx_packet_trace(bool enable)
{
    RX_trace_to_file = enable;
    CX_trace_file_open_close();
}

/******************************************************************************/
// packet_debug_tx_packet_trace()
/******************************************************************************/
void packet_debug_tx_packet_trace(bool enable)
{
    TX_trace_to_file = enable;
    CX_trace_file_open_close();
}

/****************************************************************************/
// CX_sysctl_set()
/****************************************************************************/
static void CX_sysctl_set(const char *path, int i)
{
    int  cnt;
    char buf[16];
    char *p = buf;
    int fd, res;

    if ((fd = open(path, O_WRONLY)) < 0) {
        T_E("Unable to open %s", path);
        return;
    }

    if ((cnt = snprintf(buf, 16, "%d\n", i)) >= 16) {
        T_E("int (%d) as text wider than 16 chars", i);
        goto do_exit;
    }

    while (cnt) {
        if ((res = write(fd, p, cnt)) <= 0) {
            T_E("Unable to write %d to %s", i, path);
            goto do_exit;
        }

        p += res;
        cnt -= res;
    }

do_exit:
    (void)close(fd);
}

/****************************************************************************/
// CX_ntohs()
// Takes a pointer to the first byte in a 2-byte series that should be
// converted to an u16. The 2-byte series is in network order.
/****************************************************************************/
static inline u16 CX_ntohs(const u8 *p)
{
    return (((u16)p[0] << 8) | (u16)p[1]);
}

/****************************************************************************/
// CX_ntohl()
// Takes a pointer to the first byte in a 4-byte series that should be
// converted to an u32. The 4-byte series is in network order.
/****************************************************************************/
static inline u32 CX_ntohl(u8 *p)
{
    return (((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | ((u32)p[3] << 0));
}

/****************************************************************************/
// CX_htons()
/****************************************************************************/
static inline void CX_htons(u8 *p, u16 val)
{
    p[0] = (val >>  8) & 0xFF;
    p[1] = (val >>  0) & 0xFF;
}

/****************************************************************************/
// CX_htonl()
/****************************************************************************/
static inline void CX_htonl(u8 *p, u32 val)
{
    p[0] = (val >> 24) & 0xFF;
    p[1] = (val >> 16) & 0xFF;
    p[2] = (val >>  8) & 0xFF;
    p[3] = (val >>  0) & 0xFF;
}

/****************************************************************************/
// DBG_cmd_syntax_error()
/****************************************************************************/
static void DBG_cmd_syntax_error(packet_dbg_printf_t dbg_printf, const char *fmt, ...)
{
    va_list ap;
    char    s[200] = "Command syntax error: ";
    int     len;

    len = strlen(s);

    va_start(ap, fmt);

    (void)vsnprintf(s + len, sizeof(s) - len - 1, fmt, ap);
    (void)dbg_printf("%s\n", s);

    va_end(ap);
}

/******************************************************************************/
// RX_filter_validate()
/******************************************************************************/
static BOOL RX_filter_validate(const packet_rx_filter_t *filter)
{
    if (filter->thread_prio >= VTSS_THREAD_PRIO_NA) {
        T_E("Invalid thread prio");
        return FALSE;
    }

    if (!filter->cb) {
        T_E("No callback function defined");
        return FALSE;
    }

    if (filter->prio == 0) {
        T_E("Module %s: Filter priority 0 is reserved. Use one of the PACKET_RX_FILTER_PRIO_xxx definitions", vtss_module_names[filter->modid]);
        return FALSE;
    }

    if ((filter->match & PACKET_RX_FILTER_MATCH_SRC_PORT)) {
        u32 i;
        BOOL non_zero_found = FALSE;
        for (i = 0; i < sizeof(filter->src_port_mask); i++) {
            if (filter->src_port_mask[i] != 0) {
                non_zero_found = TRUE;
                break;
            }
        }
        if (!non_zero_found) {
            T_E("Module %s: Filter is matching empty source port mask", vtss_module_names[filter->modid]);
            return FALSE;
        }
    }

    if ((filter->match & PACKET_RX_FILTER_MATCH_VID) && (filter->vid & 0xF000)) {
        T_E(".vid cannot be greater than 4095");
    }

    if ((filter->match & PACKET_RX_FILTER_MATCH_VID) && (filter->vid & filter->vid_mask)) {
        T_E("Filter contains non-zero bits in .vid that are not matched against");
        return FALSE;
    }

    if ((filter->match & PACKET_RX_FILTER_MATCH_DMAC)) {
        u32 i;
        for (i = 0; i < sizeof(filter->dmac); i++) {
            if (filter->dmac[i] & filter->dmac_mask[i]) {
                T_E("Filter contains non-zero bits in .dmac that are not matched against");
                return FALSE;
            }
        }
    }

    if ((filter->match & PACKET_RX_FILTER_MATCH_SMAC)) {
        u32 i;
        for (i = 0; i < sizeof(filter->smac); i++) {
            if (filter->smac[i] & filter->smac_mask[i]) {
                T_E("Filter contains non-zero bits in .smac that are not matched against");
                return FALSE;
            }
        }
    }

    if ((filter->match & PACKET_RX_FILTER_MATCH_ETYPE) && (filter->etype & filter->etype_mask)) {
        T_E("Filter contains non-zero bits in .etype that are not matched against");
        return FALSE;
    }

    if ((filter->match & PACKET_RX_FILTER_MATCH_IP_PROTO) && (filter->ip_proto & filter->ip_proto_mask)) {
        T_E("Filter contains bits in .ip_proto that are not matched against");
        return FALSE;
    }

    if ((filter->match & PACKET_RX_FILTER_MATCH_VLAN_TAG_ANY) && !(filter->match & PACKET_RX_FILTER_MATCH_IP_ANY)) {
        T_E("Cannot match any VLAN tag without also matching on any IP address");
        return FALSE;
    }

    if ((filter->match & PACKET_RX_FILTER_MATCH_UDP_SRC_PORT) && (filter->udp_src_port_min > filter->udp_src_port_max)) {
        T_E("Filter matches UDP source port, but min is greater than max");
        return FALSE;
    }

    if ((filter->match & PACKET_RX_FILTER_MATCH_UDP_DST_PORT) && (filter->udp_dst_port_min > filter->udp_dst_port_max)) {
        T_E("Filter matches UDP destination port, but min is greater than max");
        return FALSE;
    }

    if ((filter->match & PACKET_RX_FILTER_MATCH_TCP_SRC_PORT) && (filter->tcp_src_port_min > filter->tcp_src_port_max)) {
        T_E("Filter matches TCP source port, but min is greater than max");
        return FALSE;
    }

    if ((filter->match & PACKET_RX_FILTER_MATCH_TCP_DST_PORT) && (filter->tcp_dst_port_min > filter->tcp_dst_port_max)) {
        T_E("Filter matches TCP destination port, but min is greater than max");
        return FALSE;
    }

    if (filter->match & PACKET_RX_FILTER_MATCH_SSPID) {
        T_W("This architecture does not support match against SSPID");
    }

    if ((filter->match & PACKET_RX_FILTER_MATCH_IP_PROTO) && (filter->match & PACKET_RX_FILTER_MATCH_ETYPE) && ((filter->etype != ETYPE_IPV4 && filter->etype != ETYPE_IPV6) || filter->etype_mask != 0)) {
        T_E("Filter matches against an IP protocol, but ETYPE is not set to ETYPE_IPV4 or ETYPE_IPV6");
        return FALSE;
    }

    if (((filter->match & PACKET_RX_FILTER_MATCH_UDP_SRC_PORT) || (filter->match & PACKET_RX_FILTER_MATCH_UDP_DST_PORT)) && (filter->match & PACKET_RX_FILTER_MATCH_IP_PROTO) && (filter->ip_proto != IP_PROTO_UDP || filter->ip_proto_mask != 0)) {
        T_E("Filter matches against UDP source or destination port range, but ip_proto is not set to IP_PROTO_UDP");
        return FALSE;
    }

    if (((filter->match & PACKET_RX_FILTER_MATCH_TCP_SRC_PORT) || (filter->match & PACKET_RX_FILTER_MATCH_TCP_DST_PORT)) && (filter->match & PACKET_RX_FILTER_MATCH_IP_PROTO) && (filter->ip_proto != IP_PROTO_TCP || filter->ip_proto_mask != 0)) {
        T_E("Filter matches against TCP source or destination port range, but ip_proto is not set to IP_PROTO_TCP");
        return FALSE;
    }

    if (filter->match & PACKET_RX_FILTER_MATCH_SSPID) {
        // T_W("This architecture does not support match against SSPID"); Already printed once above.
    }

    if ((filter->match & PACKET_RX_FILTER_MATCH_DEFAULT) && ((filter->match & ~PACKET_RX_FILTER_MATCH_DEFAULT) != 0)) {
        T_E("When the PACKET_RX_FILTER_MATCH_DEFAULT is used, no other PACKET_RX_FILTER_MATCH_xxx may be used");
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************/
/******************************************************************************/
static BOOL RX_filter_remove(const void *filter_id)
{
    packet_rx_filter_item_t *l = RX_filter_list, *parent = NULL;

    // Critical region must be obtained when this function is called.
    PACKET_RX_FILTER_CRIT_ASSERT_LOCKED();

    // Find the filter.
    while (l) {
        if (l == filter_id) {
            break;
        }
        parent = l;
        l = l->next;
    }

    if (!l) {
        T_E("Filter ID not found");
        return FALSE;
    }

    // Remove it from the list.
    if (parent == NULL) {
        RX_filter_list = l->next;
    } else {
        parent->next = l->next;
    }

    RX_filter_list_changed = TRUE;

    // And free it.
    VTSS_FREE(l);

    return TRUE;
}

/******************************************************************************/
/******************************************************************************/
static BOOL RX_filter_insert(const packet_rx_filter_t *filter, void **filter_id)
{
    packet_rx_filter_item_t *l = RX_filter_list, *parent = NULL, *new_l;
    u32 i;

    // Critical region must be obtained when this function is called.
    PACKET_RX_FILTER_CRIT_ASSERT_LOCKED();

    // Figure out where to place this filter in the list, based on the priority.
    // The lower value of prio the higher priority.
    while (l) {
        if (l->filter.prio > filter->prio) {
            break;
        }
        parent = l;
        l = l->next;
    }

    // Allocate a new list item
    if ((VTSS_MALLOC_CAST(new_l, sizeof(packet_rx_filter_item_t))) == NULL) {
        T_E("VTSS_MALLOC(subscr_list_item_t) failed");
        return FALSE;
    }

    T_D("Caller: %d = %s", filter->modid, vtss_module_names[filter->modid]);

    // Copy the filter
    memcpy(&new_l->filter, filter, sizeof(new_l->filter));

    // Optimize filter. The user calls us with zeros for the places that
    // must match, but internally we use ones to be able to bit-wise AND.
    new_l->filter.vid_mask     |= 0xF000; // Still inverse polarity (match on bits set to 0), but we must only match on - at most - the 12 LSbits.
    new_l->filter.vid_mask      = ~new_l->filter.vid_mask;
    new_l->filter.etype_mask    = ~new_l->filter.etype_mask;
    new_l->filter.ip_proto_mask = ~new_l->filter.ip_proto_mask;
    new_l->filter.sspid_mask    = ~new_l->filter.sspid_mask;
    for (i = 0; i < sizeof(new_l->filter.dmac_mask); i++) {
        new_l->filter.dmac_mask[i] = ~new_l->filter.dmac_mask[i];
        new_l->filter.smac_mask[i] = ~new_l->filter.smac_mask[i];
    }

    // If user has specified one of the complex match options,
    // help him in specifying the remaining.
    if ((new_l->filter.match & PACKET_RX_FILTER_MATCH_UDP_SRC_PORT) || (new_l->filter.match & PACKET_RX_FILTER_MATCH_UDP_DST_PORT)) {
        new_l->filter.match        |= PACKET_RX_FILTER_MATCH_IP_PROTO;
        new_l->filter.ip_proto      = IP_PROTO_UDP;
        new_l->filter.ip_proto_mask = 0xFF;
    }
    if ((new_l->filter.match & PACKET_RX_FILTER_MATCH_TCP_SRC_PORT) || (new_l->filter.match & PACKET_RX_FILTER_MATCH_TCP_DST_PORT)) {
        new_l->filter.match        |= PACKET_RX_FILTER_MATCH_IP_PROTO;
        new_l->filter.ip_proto      = IP_PROTO_TCP;
        new_l->filter.ip_proto_mask = 0xFF;
    }

    // Insert the filter in the list.
    new_l->next = l;
    if (parent == NULL) {
        RX_filter_list = new_l;
    } else {
        parent->next = new_l;
    }

    RX_filter_list_changed = TRUE;

    // The filter_id may be used later on to unsubscribe.
    *filter_id = new_l;

    return TRUE;
}

/****************************************************************************/
// RX_vlan_custom_s_tag_chg_callback()
// Called by VLAN module whenever the S-Custom TPID changes.
/****************************************************************************/
static void RX_vlan_s_custom_etype_change_callback(mesa_etype_t tpid)
{
    T_D("New TPID: 0x%04x", tpid);
    RX_vlan_s_custom_tpid = tpid;
}

/****************************************************************************/
// RX_is_ip_any_etype()
/****************************************************************************/
static BOOL RX_is_ip_any_etype(mesa_etype_t etype)
{
#if defined(VTSS_SW_OPTION_IPV6)
    return (etype == ETYPE_IPV4 || etype == ETYPE_ARP || etype == ETYPE_IPV6);
#else
    return (etype == ETYPE_IPV4 || etype == ETYPE_ARP);
#endif /* defined(VTSS_SW_OPTION_IPV6) */
}

/****************************************************************************/
// RX_filter_list_release()
/****************************************************************************/
static void RX_filter_list_release(packet_rx_filter_list_t *filter_list, BOOL crit_taken)
{
    if (!filter_list) {
        return;
    }

    if (crit_taken) {
        PACKET_RX_FILTER_CRIT_ASSERT_LOCKED();
    } else {
        PACKET_RX_FILTER_CRIT_ENTER();
    }

    T_RG(TRACE_GRP_RX, "Current ref-count on %p = %u\n", filter_list, filter_list->ref_cnt);

    if (filter_list->ref_cnt == 0) {
        T_EG(TRACE_GRP_RX, "Ref count is already zero on filter %p", filter_list);
    } else if (--filter_list->ref_cnt == 0) {
        T_RG(TRACE_GRP_RX, "Releasing filter %p", filter_list);
        RX_filter_lists_alive--;
        VTSS_FREE(filter_list);
    }

    if (!crit_taken) {
        PACKET_RX_FILTER_CRIT_EXIT();
    }
}

/****************************************************************************/
// RX_filter_list_get()
/****************************************************************************/
static void RX_filter_list_get(packet_dscr_t *packet_dscr)
{
    static packet_rx_filter_list_t *cached_filter_list;
    packet_rx_filter_item_t        *src, *dst;
    u32                            item_cnt;

    // The global RX_filter_list is alive and modules may add and remove
    // new filters while we're receiving frames.
    // Therefore, we have our own, cached version here. It gets updated
    // whenever the global one has changed and we receive a new frame.

    // The cached version is ref-counted and follows a frame, and only
    // when a frame has been handled, will the ref-cnt decrease, which in
    // turn may cause it to be freed.

    PACKET_RX_FILTER_CRIT_ENTER();
    if (RX_filter_list_changed) {
        T_RG(TRACE_GRP_RX, "Updating local subscriber list");

        // The list has changed. Release our own ref to the old one.
        // Last arg in the next call indicates whether PACKET_RX_FILTER_CRIT_ENTER()
        // has already been called.
        RX_filter_list_release(cached_filter_list, TRUE);

        // We need to figure out how many items we need.
        item_cnt = 0;
        src = RX_filter_list;
        while (src) {
            item_cnt++;
            src = src->next;
        }

        if (item_cnt > 0) {
            if ((cached_filter_list = (packet_rx_filter_list_t *)VTSS_MALLOC(sizeof(packet_rx_filter_list_t) + item_cnt * sizeof(packet_rx_filter_item_t))) == NULL) {
                T_EG(TRACE_GRP_RX, "Unable to allocate filter list");
            }
        } else {
            cached_filter_list = NULL;
        }

        // Copy items.
        if (cached_filter_list) {
            RX_filter_lists_alive++;
            RX_filter_lists_total++;

            // We allocated room for both the container and the items. Make the
            // items point to the first byte after the container.
            cached_filter_list->items = (packet_rx_filter_item_t *)((u8 *)cached_filter_list + sizeof(packet_rx_filter_list_t));
            cached_filter_list->item_cnt = item_cnt;
            cached_filter_list->ref_cnt = 1; // We own it.

            src = RX_filter_list;
            dst = cached_filter_list->items;
            while (src) {
                *dst = *src;
                if (src->next) {
                    dst->next = (packet_rx_filter_item_t *)((u8 *)dst + sizeof(packet_rx_filter_item_t));
                }

                src = src->next;
                dst = dst->next;
            }
        }

        RX_filter_list_changed = FALSE;
    }

    // Pass a reference to #packet_dscr while increasing the ref-cnt.
    packet_dscr->filter_list = cached_filter_list;
    packet_dscr->next_filter = cached_filter_list ? cached_filter_list->items : NULL;
    packet_dscr->next_default_filter = packet_dscr->next_filter;
    if (packet_dscr->filter_list) {
        packet_dscr->filter_list->ref_cnt++;
    }

    PACKET_RX_FILTER_CRIT_EXIT();
}

/****************************************************************************/
// RX_count()
/****************************************************************************/
static void RX_count(const packet_dscr_t *packet_dscr)
{
    const mesa_packet_rx_info_t *rx_info = &packet_dscr->rx_info;
    u32                         port_bucket, prio_bucket, qu;

    // Count the frame
    if (rx_info->tag.pcp >= VTSS_PRIOS) {
        prio_bucket = VTSS_PRIOS - 1; // Count in the next-to-last-bucket if the priority field is out of bounds.
    } else {
        prio_bucket = rx_info->tag.pcp;
    }

    if (rx_info->port_no >= CX_port_cnt) {
        // Unknown source port. Count it in the last bucket.
        port_bucket = CX_port_cnt;
    } else {
        port_bucket = rx_info->port_no;
    }

    // The extration queue is indicated by the highest set bit of xtr_qu_mask.
    qu = MIN(ARRSZ(RX_xtr_qu_counters) - 1, 31 - VTSS_OS_CLZ(rx_info->xtr_qu_mask));

    // Update port counters
    PACKET_CX_COUNTER_CRIT_ENTER();
    CX_port_counters.rx_pkts[port_bucket][prio_bucket]++;
    RX_xtr_qu_counters[qu]++;
    PACKET_CX_COUNTER_CRIT_EXIT();
}

/******************************************************************************/
// RX_tag_type_to_str()
/******************************************************************************/
static const char *RX_tag_type_to_str(mesa_tag_type_t tag_type)
{
    switch (tag_type) {
    case MESA_TAG_TYPE_UNTAGGED:
        return "Untagged";

    case MESA_TAG_TYPE_C_TAGGED:
        return "C";

    case MESA_TAG_TYPE_S_TAGGED:
        return "S";

    case MESA_TAG_TYPE_S_CUSTOM_TAGGED:
        return "S-custom";

    default:
        return "Unknown";
    }
}

/******************************************************************************/
// RX_sflow_type_to_str()
/******************************************************************************/
static const char *RX_sflow_type_to_str(mesa_sflow_type_t sflow_type)
{
    switch (sflow_type) {
    case MESA_SFLOW_TYPE_NONE:
        return "none";

    case MESA_SFLOW_TYPE_RX:
        return "Rx";

    case MESA_SFLOW_TYPE_TX:
        return "Tx";

    case MESA_SFLOW_TYPE_ALL:
        return "All";

    default:
        return "Unknown";
    }
}

/****************************************************************************/
// RX_trace()
/****************************************************************************/
static void RX_trace(const packet_dscr_t *packet_dscr)
{
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO)
    const mesa_packet_rx_info_t *rx_info = &packet_dscr->rx_info;
    const u8                    *frm_ptr = packet_dscr->frm_ptr;
    char                        dmac_str[18], smac_str[18];
#endif

    T_NG(    TRACE_GRP_RX, "Packet (%u bytes shown):", rx_info->length);
    T_NG_HEX(TRACE_GRP_RX, frm_ptr, rx_info->length);
    if (RX_trace_to_file) {
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO)
        CX_trace_to_file(frm_ptr, rx_info->length);
#endif
    }

    // Avoid Lint Warning 436: Apparent preprocessor directive in invocation of macro 'T_IG'
    /*lint --e{436} */
    T_IG(TRACE_GRP_RX,
         "\n"
         "DMAC                   = %s\n"
         "SMAC                   = %s\n"
         "EtherType              = 0x%04x\n"
         "Length (w/o FCS)       = %u\n"
         "IFH.iport              = %u\n"
         "   .uport              = %u\n"
         "   .tag_type           = %s\n"
         "   .class_tag.pcp      = %u\n"
         "             .dei      = %u\n"
         "             .vid      = %u\n"
         "   .strip_tag.tpid     = 0x%04x\n"
         "             .pcp      = %u\n"
         "             .dei      = %u\n"
         "             .vid      = %u\n"
         "   .qu_mask            = 0x%x\n"
         "   .cos                = %u\n"
         "   .acl_hit            = %u\n"
         "   .tstamp_id          = %u\n"
         "   .hw_tstamp          = 0x%08x\n"
#if defined(VTSS_SW_OPTION_SFLOW)
         "   .sflow_type         = %s\n"
         "   .sflow_port         = %u\n"
#endif /* defined(VTSS_SW_OPTION_SFLOW) */
         "   .iflow_id           = %u\n"
         "   .rb_port_a          = %d\n"
         "   .rb_tagged          = %d\n"
         "hints.tag_mismatch     = %d\n"
         "     .frame_mismatch   = %d\n"
         "     .vid_mismatch     = %d",
         misc_mac_txt(&frm_ptr[DMAC_POS], dmac_str),
         misc_mac_txt(&frm_ptr[SMAC_POS], smac_str),
         packet_dscr->etype,
         rx_info->length,
         rx_info->port_no,
         iport2uport(rx_info->port_no),
         RX_tag_type_to_str(rx_info->tag_type),
         rx_info->tag.pcp,
         rx_info->tag.dei,
         rx_info->tag.vid,
         rx_info->stripped_tag.tpid, // tpid == 0x00 means that no tag was stripped
         rx_info->stripped_tag.pcp,
         rx_info->stripped_tag.dei,
         rx_info->stripped_tag.vid,
         rx_info->xtr_qu_mask,
         rx_info->cos,
         rx_info->acl_hit,
         rx_info->tstamp_id,
         rx_info->hw_tstamp,
#if defined(VTSS_SW_OPTION_SFLOW)
         RX_sflow_type_to_str(rx_info->sflow_type),
         rx_info->sflow_port_no,
#endif /* defined(VTSS_SW_OPTION_SFLOW) */
         rx_info->iflow_id,
         rx_info->rb_port_a,
         rx_info->rb_tagged,
         (rx_info->hints & MESA_PACKET_RX_HINTS_VLAN_TAG_MISMATCH)   != 0,
         (rx_info->hints & MESA_PACKET_RX_HINTS_VLAN_FRAME_MISMATCH) != 0,
         (rx_info->hints & MESA_PACKET_RX_HINTS_VID_MISMATCH)        != 0);
}

/****************************************************************************/
// RX_pop_tag()
/****************************************************************************/
static u32 RX_pop_tag(packet_dscr_t *packet)
{
    u32 tag = CX_ntohl(&packet->frm_ptr[ETYPE_POS]);
    u8  *src = packet->frm_ptr + ETYPE_POS     - 1;
    u8  *dst = packet->frm_ptr + ETYPE_POS + 4 - 1;
    int i;

    // Strip VLAN tag by moving DMAC and SMAC four bytes ahead.
    // Gotta copy byte by byte due to overlapping areas.
    for (i = 0; i < (2 * (VTSS_MAC_ADDR_SZ_BYTES)); i++) {
        *(dst--) = *(src--);
    }

    packet->frm_ptr        += 4;
    packet->act_len        -= 4;
    packet->rx_info.length -= 4;

    // Allows caller to re-insert the tag that we just stripped
    return tag;
}

/****************************************************************************/
// RX_push_tag()
/****************************************************************************/
static void RX_push_tag(packet_dscr_t *packet, u32 tag)
{
    u8  *src = packet->frm_ptr;
    u8  *dst = packet->frm_ptr - 4;
    int i;

    // First move MAC addresses four bytes earlier
    // in the frame (there is room, since it's guranteed
    // that RX_pop_tag() indeed has been called prior
    // to calling this function).
    for (i = 0; i < (2 * (VTSS_MAC_ADDR_SZ_BYTES)); i++) {
        *(dst++) = *(src++);
    }

    packet->frm_ptr        -= 4;
    packet->act_len        += 4;
    packet->rx_info.length += 4;

    // Then insert the original tag into the frame
    CX_htonl(&packet->frm_ptr[ETYPE_POS], tag);
}

/****************************************************************************/
// RX_do_callback()
// Return values:
//   0: Callback handler determined that it's OK to pass on this frame on to others (not consumed)
//   1: Callback handler determined that it's not OK to pass this frame on to others (consumed)
//   2: Callback handler determined to take over the frame pointer. NO LONGER SUPPORTED
/****************************************************************************/
static int RX_do_callback(packet_rx_dispatch_thread_state_t *thread_state, packet_dscr_t *packet_dscr)
{
    int                      result   = 0; // Pass it on to the next by default
    vtss_tick_count_t        tick_cnt = vtss_current_time();
    packet_rx_filter_t       *filter  = &packet_dscr->current_filter->filter;
    packet_module_counters_t *cntrs   = &CX_module_counters[filter->modid];
    u64                      max_time_ms;

    // Update statistics for this filter.
    cntrs->rx_bytes += packet_dscr->rx_info.length;
    cntrs->rx_pkts++;

    if (filter->cb) {
        // Use the normal callback function.
        if (filter->cb(filter->contxt, packet_dscr->frm_ptr, &packet_dscr->rx_info)) {
            result = 1; // Consumed
        }
    } else {
        // Since we can't call subscriber, simply tell caller that we have consumed the frame.
        // That is, don't pass it on to others.
        result = 1; // Simulate consumed
    }

    tick_cnt = vtss_current_time() - tick_cnt;
    if (tick_cnt > cntrs->longest_rx_callback_ticks) {
        cntrs->longest_rx_callback_ticks = tick_cnt;
    }

    // We don't care that the thread_state mutex hasn't been taken when updating this
    if (tick_cnt > thread_state->longest_rx_callback_ticks) {
        thread_state->longest_rx_callback_ticks = tick_cnt;
    }

    if (filter->modid == VTSS_MODULE_ID_MSG || filter->modid == VTSS_MODULE_ID_TOPO) {
        // The message module is pretty busy when adding 16 switches to a stack,
        // so allow it a bit more time.
        max_time_ms = 5000;
    } else {
        max_time_ms = 3000;
    }

    if (tick_cnt > VTSS_OS_MSEC2TICK(max_time_ms)) {
        T_I("Module %s has spent more than " VPRI64u" msecs (" VPRI64u" msecs) in its Packet Rx callback", vtss_module_names[filter->modid], max_time_ms, VTSS_OS_TICK2MSEC(tick_cnt));
    }

    return result;
}

/****************************************************************************/
// RX_thread_frm_free()
/****************************************************************************/
static void RX_thread_frm_free(packet_dscr_t *packet_dscr)
{
    static bool     max_has_been_hit;
    static uint32_t missing_sem_post_cnt;

    // Free the frame pointer and increase the semaphore count since we can now
    // handle one more frame.
    T_RG(TRACE_GRP_RX, "Freeing the frame");
    VTSS_FREE(packet_dscr->opaque);

    (void)vtss_mutex_lock(&RX_low_level_mutex);
    if (RX_outstanding-- == RX_BUF_CNT) {
        max_has_been_hit = true;
    }

    // Make hysteresis so that if max. outstanding has been hit, we don't post
    // RX_packet_sem until half of them are gone. This makes
    // RX_thread() wait until we add some bufs. It's then up to the kernel to
    // discard the excess frames.
    // If we didn't do this, then as soon as a module has processed a frame,
    // will a thread switch to the RX_thread() occur and a lot of overhead is
    // involved in that to no avail.
    if (max_has_been_hit) {
        missing_sem_post_cnt++;

        if (RX_outstanding == RX_BUF_CNT / 2) {
            vtss_sem_post(&RX_packet_sem, missing_sem_post_cnt);
            missing_sem_post_cnt = 0;
            max_has_been_hit = false;
        }
    } else {
        vtss_sem_post(&RX_packet_sem);
    }

    vtss_mutex_unlock(&RX_low_level_mutex);
}

/****************************************************************************/
// RX_callback()
/****************************************************************************/
static void RX_callback(packet_rx_dispatch_thread_state_t *thread_state, packet_dscr_t *packet_dscr)
{
    u32 tag = 0;
    int callback_result;
    BOOL free_packet, free_filter;

    if (packet_dscr->strip_outer_tag) {
        // This is a double-tagged frame and the user subscribes to such frames.
        // He expects both tags to be stripped. The original outer tag is already stripped
        // and now we strip the original inner (which is the current outer).
        tag = RX_pop_tag(packet_dscr);
    }

    callback_result = RX_do_callback(thread_state, packet_dscr);
    if (callback_result == 0) {
        // Packet handled, but not consumed. Allow others to get it too.
        // Restore a possible tag before matching again.
        if (packet_dscr->strip_outer_tag) {
            RX_push_tag(packet_dscr, tag);
        }

        if (RX_match_next(packet_dscr)) {
            // Somebody else got this frame. Don't free anything.
            free_packet = FALSE;
            free_filter = FALSE;
        } else {
            // No more subscribers or an error occurred. Either way,
            // free the frame.
            free_packet = TRUE;
            free_filter = TRUE;
        }
    } else if (callback_result == 1) {
        // Packet consumed. Don't call anyone else. Free both packet and
        // decrease filter-list ref-cnt.
        free_packet = TRUE;
        free_filter = TRUE;
    } else {
        T_EG(TRACE_GRP_RX, "Unknown result (%d)", callback_result);
    }

    if (free_packet) {
        RX_thread_frm_free(packet_dscr);
    }

    if (free_filter) {
        RX_filter_list_release(packet_dscr->filter_list, FALSE);
    }
}

/****************************************************************************/
// RX_dispatch_thread()
// Common thread function that dispatches frames to registered user modules
// on a given thread priority.
/****************************************************************************/
static void RX_dispatch_thread(vtss_addrword_t data)
{
    packet_rx_dispatch_thread_state_t *thread_state = (packet_rx_dispatch_thread_state_t *)data;
    packet_dscr_t                     packet_dscr;
    BOOL                              work;

    while (1) {
        // Wait for packets in the Rx dispatch FIFO belonging to this thread.
        // This waits for bit 0 in the flags to become 1 and clears them when that happens
        (void)vtss_flag_wait(&thread_state->flag, 1, VTSS_FLAG_WAITMODE_AND_CLR);

        PACKET_RX_DISPATCH_THREAD_CRIT_ENTER();

        while (vtss_fifo_cp_cnt(&thread_state->fifo) != 0) {
            // Get the next packet from the FIFO.
            work = vtss_fifo_cp_rd(&thread_state->fifo, &packet_dscr) == VTSS_RC_OK;

            if (work) {
                // Update counters while we have the mutex.
                thread_state->rx_pkts++;
                thread_state->rx_bytes += packet_dscr.act_len;
            }

            PACKET_RX_DISPATCH_THREAD_CRIT_EXIT();

            if (work) {
                T_RG(TRACE_GRP_RX, "opaque = %p\nfrm_ptr = %p\nact_len = %u\nfilter_list = %p\nmatch_cnt = %u\nmatch_default_cnt = %u\ncurrent_filter = %p\nnext_filter = %p\nnext_default_filter = %p\n",
                     packet_dscr.opaque,
                     packet_dscr.frm_ptr,
                     packet_dscr.act_len,
                     packet_dscr.filter_list,
                     packet_dscr.match_cnt,
                     packet_dscr.match_default_cnt,
                     packet_dscr.current_filter,
                     packet_dscr.next_filter,
                     packet_dscr.next_default_filter);
                RX_callback(thread_state, &packet_dscr);
            } else {
                T_EG(TRACE_GRP_RX, "Unable to get frame descriptor from FIFO");
            }

            // Before checking the fifo again, take the mutex again
            PACKET_RX_DISPATCH_THREAD_CRIT_ENTER();
        }

        // Before waiting for the thread flag, exit the mutex.
        PACKET_RX_DISPATCH_THREAD_CRIT_EXIT();
    }
}

/****************************************************************************/
// RX_dispatch_thread_create()
/****************************************************************************/
static BOOL RX_dispatch_thread_create(packet_rx_dispatch_thread_state_t *thread_state, vtss_thread_prio_t thread_prio)
{
    T_DG(TRACE_GRP_RX, "Enter");

    // Create a FIFO, initially one quarter of the number of Rx buffers, and let
    // it dynamically grow to the number of Rx buffers one quarter at a time.
    if (vtss_fifo_cp_init(&thread_state->fifo, sizeof(packet_dscr_t), RX_BUF_CNT / 4, RX_BUF_CNT, RX_BUF_CNT / 4, TRUE) != VTSS_RC_OK) {
        T_E("Unable to create FIFO for Rx Dispatch thread for prio = %d", thread_prio);
        return FALSE;
    }

    // Initialize the flag used to signal the FIFO is non-empty.
    vtss_flag_init(&thread_state->flag);

    // And finally create the thread.
    snprintf(thread_state->thread_name, sizeof(thread_state->thread_name), "Rx Dispatch (%s)", vtss_thread_prio_to_txt(thread_prio));
    vtss_thread_create(thread_prio,
                       RX_dispatch_thread,
                       thread_state,
                       thread_state->thread_name,
                       nullptr,
                       0,
                       &thread_state->thread_handle,
                       &thread_state->thread_state);

    T_DG(TRACE_GRP_RX, "Exit");

    return TRUE;
}

/****************************************************************************/
// RX_dispatch_thread_put()
/****************************************************************************/
static BOOL RX_dispatch_thread_put(packet_dscr_t *packet_dscr)
{
    vtss_thread_prio_t                thread_prio = packet_dscr->current_filter->filter.thread_prio;
    packet_rx_dispatch_thread_state_t *thread_state = &RX_dispatch_thread_states[thread_prio];
    BOOL                              result = TRUE;

    PACKET_RX_DISPATCH_THREAD_CRIT_ENTER();

    // Check to see if we've created a thread with this priority.
    if (!thread_state->thread_handle) {
        // We haven't. Do it.
        if (!RX_dispatch_thread_create(thread_state, thread_prio)) {
            result = FALSE;
            goto do_exit;
        }
    }

    // Thread (now) exists, so post the packet to it.
    if (vtss_fifo_cp_wr(&thread_state->fifo, packet_dscr) == VTSS_RC_OK) {
        // The frame is now stored in the FIFO.
        // Signal the flag to wake up the dispatch thread.
        vtss_flag_setbits(&thread_state->flag, 1);
    } else {
        T_EG(TRACE_GRP_RX, "Unable to store frame in dispatch thread's FIFO for prio = %d", thread_prio);
        result = FALSE;
    }

do_exit:
    PACKET_RX_DISPATCH_THREAD_CRIT_EXIT();
    return result;
}

/****************************************************************************/
// RX_match_next()
// Return value:
//   TRUE:
//     The packet indeed has been successfully stored on a dispatch thread's
//     FIFO (because of a match). The caller must not free the packet, nor
//     decrease ref-cnt on the filter in that case.
//   FALSE:
//     Indicates that 1) it couldn't dispatch the frame due to an error or
//     2) there were no more matches. In either case it must free the frame
//     and decrease the ref-cnt on the filter.
/****************************************************************************/
static BOOL RX_match_next(packet_dscr_t *packet_dscr)
{
    mesa_packet_rx_info_t *rx_info = &packet_dscr->rx_info;
    u8                    *frm_ptr = packet_dscr->frm_ptr;
    u32                   i;

    // Go on with the next filter
    while (packet_dscr->next_filter) {
        packet_rx_filter_t *filter = &packet_dscr->next_filter->filter;
        u32                match  = filter->match;

        packet_dscr->strip_outer_tag = FALSE;

        T_RG(TRACE_GRP_RX, "%s: Considering match = 0x%08x", vtss_module_names[filter->modid], match);

        if (packet_dscr->sflow_subscribers_only && !(match & PACKET_RX_FILTER_MATCH_SFLOW)) {
            // When sflow_subscribers_only is TRUE, only subscribers matching
            // the sflow flag may get the packet because the src_port
            // may be invalid if received on internal ports on JR-48, and
            // because the frame is a random sample of the traffic on the
            // front ports.
            goto no_match;
        }

        if (rx_info->length + FCS_SIZE_BYTES > filter->mtu) {
            // Frame is longer than what the module supports.
            goto no_match;
        }

        if (match & PACKET_RX_FILTER_MATCH_SRC_PORT) {
            if (rx_info->port_no != VTSS_PORT_NO_NONE && VTSS_PORT_BF_GET(filter->src_port_mask, rx_info->port_no) == 0) {
                goto no_match; // Hard to code without goto statements (matching = FALSE; break; in all checks)
            }
        }

        if (match & PACKET_RX_FILTER_MATCH_ACL) {
            if (!rx_info->acl_hit) {
                goto no_match;
            }
        }

        if (match & PACKET_RX_FILTER_MATCH_VID) {
            if ((rx_info->tag.vid & filter->vid_mask) != filter->vid) {
                goto no_match;
            }
        }

        if (match & PACKET_RX_FILTER_MATCH_DMAC) {
            for (i = 0; i < sizeof(filter->dmac); i++) {
                if ((frm_ptr[DMAC_POS + i] & filter->dmac_mask[i]) != filter->dmac[i]) {
                    goto no_match;
                }
            }
        }

        if (match & PACKET_RX_FILTER_MATCH_SMAC) {
            for (i = 0; i < sizeof(filter->smac); i++) {
                if ((frm_ptr[SMAC_POS + i] & filter->smac_mask[i]) != filter->smac[i]) {
                    goto no_match;
                }
            }
        }

        if (match & PACKET_RX_FILTER_MATCH_ETYPE) {
            if ((packet_dscr->etype & filter->etype_mask) != filter->etype) {
                goto no_match;
            }
        }

        if (match & PACKET_RX_FILTER_MATCH_IP_PROTO) {
            u16 hlen;

            // No need to check for ETYPE == ETYPE_IPV4 || ETYPE == ETYPE_IPV6,
            // since it's already embedded in the filter (required and checked in RX_filter_validate())

            if (packet_dscr->etype == ETYPE_IPV4) {
                // IPv4
                if ((frm_ptr[IPV4_PROTO_POS] & filter->ip_proto_mask) != filter->ip_proto || (frm_ptr[IP_VER_POS] & 0xF0) != 0x40) {
                    goto no_match;
                }

                hlen = 4 * (frm_ptr[IPV4_HLEN_POS] & 0xF);
            } else {
                // IPv6. Only check first Next Header field, that is, we don't support any extension headers.
                if ((frm_ptr[IPV6_PROTO_POS] & filter->ip_proto_mask) != filter->ip_proto || (frm_ptr[IP_VER_POS] & 0xF0) != 0x60) {
                    goto no_match;
                }

                hlen = 40;
            }

            if (match & PACKET_RX_FILTER_MATCH_UDP_SRC_PORT) {
                u16 udp_src_port;

                udp_src_port = (frm_ptr[UDP_SRC_PORT_POS(hlen)] << 8) | (frm_ptr[UDP_SRC_PORT_POS(hlen) + 1]);
                if (udp_src_port < filter->udp_src_port_min || udp_src_port > filter->udp_src_port_max) {
                    goto no_match;
                }
            }

            if (match & PACKET_RX_FILTER_MATCH_UDP_DST_PORT) {
                u16 udp_dst_port;

                udp_dst_port = (frm_ptr[UDP_DST_PORT_POS(hlen)] << 8) | (frm_ptr[UDP_DST_PORT_POS(hlen) + 1]);
                if (udp_dst_port < filter->udp_dst_port_min || udp_dst_port > filter->udp_dst_port_max) {
                    goto no_match;
                }
            }

            if (match & PACKET_RX_FILTER_MATCH_TCP_SRC_PORT) {
                u16 tcp_src_port;

                tcp_src_port = (frm_ptr[TCP_SRC_PORT_POS(hlen)] << 8) | (frm_ptr[TCP_SRC_PORT_POS(hlen) + 1]);
                if (tcp_src_port < filter->tcp_src_port_min || tcp_src_port > filter->tcp_src_port_max) {
                    goto no_match;
                }
            }

            if (match & PACKET_RX_FILTER_MATCH_TCP_DST_PORT) {
                u16 tcp_dst_port;

                tcp_dst_port = (frm_ptr[TCP_DST_PORT_POS(hlen)] << 8) | (frm_ptr[TCP_DST_PORT_POS(hlen) + 1]);
                if (tcp_dst_port < filter->tcp_dst_port_min || tcp_dst_port > filter->tcp_dst_port_max) {
                    goto no_match;
                }
            }
        }

        if (match & PACKET_RX_FILTER_MATCH_IP_ANY) {
            // This is hard to grasp in negative logic, hence the empty if().
            if (packet_dscr->outer_is_ip_any || (packet_dscr->outer_is_vlan_tag_and_inner_is_ip_any && (match & PACKET_RX_FILTER_MATCH_VLAN_TAG_ANY))) {
                // Either [outer is any IP frame] or [outer is a VLAN tag and the inner is any IP frame and the user module wants this type of frames]
                packet_dscr->strip_outer_tag = packet_dscr->outer_is_vlan_tag_and_inner_is_ip_any && (match & PACKET_RX_FILTER_MATCH_VLAN_TAG_ANY);
            } else {
                goto no_match;
            }
        }

#if defined(VTSS_SW_OPTION_SFLOW)
        if (match & PACKET_RX_FILTER_MATCH_SFLOW) {
            if (rx_info->sflow_type == MESA_SFLOW_TYPE_NONE) {
                goto no_match;
            }
        }
#endif /* VTSS_SW_OPTION_SFLOW */

        if (match & PACKET_RX_FILTER_MATCH_DEFAULT) {
            // Skip this one for now. It's only used if no other filters matched.
            goto no_match;
        }

        // If we get here, it's either because the match filter was empty,
        // i.e. match == PACKET_RX_FILTER_MATCH_ANY, or the filter matched
        // all the way through.
        packet_dscr->match_cnt++;

        // Match!!!
        // Save a ref to the filter we want to call the subscriber for,
        // advance the iteration filter and post the frame on the relevant thread.
        packet_dscr->current_filter = packet_dscr->next_filter;
        packet_dscr->next_filter    = packet_dscr->next_filter->next;
        T_DG(TRACE_GRP_RX, "%s: Match", vtss_module_names[filter->modid]);
        return RX_dispatch_thread_put(packet_dscr);

no_match:
        T_RG(TRACE_GRP_RX, "%s: No match", vtss_module_names[filter->modid]);
        packet_dscr->next_filter = packet_dscr->next_filter->next;
        continue;
    }

    // If we get here, the packet has not been dispatched to any (further) subscriber(s).
    // If the match_cnt is non-zero, it has already been dispatched at least once.
    // If the match_cnt is zero, it has never been dispatched, so we need to run through
    // the filters again and look for default-subscribers and dispatch the frame to
    // such threads.
    if (packet_dscr->match_cnt) {
        // Has been dispatched to at least one subscriber, but there are no more
        // to go, so it's time to release the frame and reference to the filter.
        return FALSE;
    }

    // Look for (further) default-subscribers.
    packet_dscr->strip_outer_tag = FALSE;
    while (packet_dscr->next_default_filter) {
        packet_rx_filter_t *filter = &packet_dscr->next_default_filter->filter;
        u32                match  = filter->match;

        if (match & PACKET_RX_FILTER_MATCH_DEFAULT) {
            packet_dscr->match_default_cnt++;

            // Match!!!
            // Advance the filter and post the frame on the relevant thread.
            packet_dscr->current_filter = packet_dscr->next_default_filter;
            packet_dscr->next_default_filter = packet_dscr->next_default_filter->next;
            T_DG(TRACE_GRP_RX, "%s: Match", vtss_module_names[filter->modid]);
            return RX_dispatch_thread_put(packet_dscr);
        }

        packet_dscr->next_default_filter = packet_dscr->next_default_filter->next;
    }

    // If we get here, there were no more default-subscribers - if any.
    // Update "no-subscriber" statistics if there were no subscribers at all.
    T_RG(TRACE_GRP_RX, "No matches at all");

    PACKET_RX_FILTER_CRIT_ENTER();
    if (packet_dscr->match_default_cnt == 0) {
        RX_bytes_no_subscribers += rx_info->length;
        RX_pkts_no_subscribers++;
    }
    PACKET_RX_FILTER_CRIT_EXIT();

    // It's time to release buffers and reference to filter.
    return FALSE;
}

/****************************************************************************/
// RX_dispatch_init()
// Initialize #packet_dscr with further info about the frame before
// attempting to find a subscriber (using RX_match_next()).
// This function takes ownership of the packet_dscr frame buffer and will
// hand it over to subscribers if any (otherwise free whatever is to free).
/****************************************************************************/
static void RX_dispatch_init(packet_dscr_t *packet_dscr)
{
    mesa_packet_rx_info_t *rx_info = &packet_dscr->rx_info;
    u8                    *frm_ptr = packet_dscr->frm_ptr;
    BOOL                  free_packet;

    packet_dscr->filter_list = NULL;
    packet_dscr->sflow_subscribers_only = FALSE;
#if defined(VTSS_SW_OPTION_SFLOW)
    // One more case, where the frame must only come to sFlow subscribers:
    // If the extraction queue mask from the extraction header indicates that the only
    // reason for sending the frame to the CPU was for sFlow sampling, we need
    // to prevent it from getting to other listeners.
    // This implies that the sFlow Rx queue must only be used for sFlow frames, nothing else.
    if (rx_info->sflow_type != MESA_SFLOW_TYPE_NONE) {
        if ((rx_info->xtr_qu_mask & ~VTSS_BIT(PACKET_XTR_QU_SFLOW)) == 0) {
            // Only got here due to sFlow sampling. Prevent it from being sent to other modules.
            packet_dscr->sflow_subscribers_only = TRUE;
        }
    }
#endif /* VTSS_SW_OPTION_SFLOW */

    packet_dscr->etype = CX_ntohs(&frm_ptr[ETYPE_POS]);
    packet_dscr->outer_is_vlan_tag_and_inner_is_ip_any = FALSE;
    if (rx_info->tag_type != MESA_TAG_TYPE_UNTAGGED) {
        // The frame originally had an outer tag. Check to see if it has an inner tag as well,
        // and if so check to see if the etype below that inner tag is an IP-etype.
        if (packet_dscr->etype == 0x8100 || packet_dscr->etype == 0x88a8 || packet_dscr->etype == RX_vlan_s_custom_tpid) {
            // The frame indeed had an inner tag. Look for etype past that inner tag.
            packet_dscr->outer_is_vlan_tag_and_inner_is_ip_any = RX_is_ip_any_etype(CX_ntohs(&frm_ptr[ETYPE_INNER_POS]));
        }
    }

    packet_dscr->outer_is_ip_any = RX_is_ip_any_etype(packet_dscr->etype);
    packet_dscr->match_cnt = 0;
    packet_dscr->match_default_cnt = 0;

    // Get a reference to the most recent Rx filter list while increasing ref_cnt on it.
    RX_filter_list_get(packet_dscr);

    if (rx_info->port_no == VTSS_PORT_NO_NONE) {
        T_EG(TRACE_GRP_RX, "Port number must not be VTSS_PORT_NO_NONE");
        free_packet = TRUE;
        goto do_exit;
    }

    // Count the frame
    RX_count(packet_dscr);

    // Output trace if enabled
    if (CX_stack_trace_ena || packet_dscr->etype != 0x8880) {
        RX_trace(packet_dscr);
    }

    // When RX_match_next() returns TRUE, ownership of frame and filter reference
    // counting is transferred to another thread. If it returns FALSE, either
    // an error occurred, or there were no matches at all - and no default-matches.
    // In the latter case, we must free the packet.
    free_packet = !RX_match_next(packet_dscr);

do_exit:
    if (free_packet) {
        // Free frame pointer...
        RX_thread_frm_free(packet_dscr);
        // ...and release reference to filter list.
        RX_filter_list_release(packet_dscr->filter_list, FALSE);
    }
}

/****************************************************************************/
// CX_socket_promisc_set()
/****************************************************************************/
static mesa_rc CX_socket_promisc_set(int fd, int ifindex, const char *device_name)
{
    struct packet_mreq mreq;

    memset(&mreq, 0, sizeof(mreq));
    mreq.mr_ifindex = ifindex;
    mreq.mr_type = PACKET_MR_PROMISC;

    T_I("Setting %s to promiscuous", device_name);
    if (setsockopt(fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != 0) {
        T_E("Unable to enter promiscuous mode: %s", strerror(errno));
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

/****************************************************************************/
// CX_socket_rcvbuf_set()
// Try to use "sysctl -a | grep net.core.rmem"
/****************************************************************************/
static mesa_rc CX_socket_rcvbuf_set(int fd, const char *device_name)
{
    // Set the sk_rcvbuf to the size of maximum frame it can received (including
    // FCS and excluding IFH) multiplied by number of frames. Already the kernel
    // takes care of other headers that are appended to the skb and frame,
    // because it is multiplying this value by 2. This value can't be bigger
    // than rmem_max.
    // See also rmem_max configuration somewhere else in this file.
    int       sz     = fast_cap(MESA_CAP_PORT_FRAME_LENGTH_MAX) * RX_BUF_CNT;
    socklen_t act_sz = sizeof(sz);

    T_I("Setting SO_RCVBUF to %d bytes on %s", sz, device_name);
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &sz, act_sz) != 0) {
        T_E("Unable to change IFH socket's receive buffer size to %d: %s", sz, strerror(errno));
        return VTSS_RC_ERROR;
    }

    if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &sz, &act_sz) != 0) {
        T_E("getsockopt() failed: %s", strerror(errno));
        return VTSS_RC_ERROR;
    }

    T_I("SO_RCVBUF = %d bytes", sz);

    return VTSS_RC_OK;
}

/******************************************************************************/
// CX_socket_up()
/******************************************************************************/
static void CX_socket_up(const char *device_name)
{
    struct ifreq ifr;
    int          sockfd;

    T_I("Setting %s UP", device_name);
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        T_E("Unable to UP %s: %s", device_name, strerror(errno));
        return;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, device_name, IFNAMSIZ);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) >= 0) {
        strncpy(ifr.ifr_name, device_name, IFNAMSIZ);
        ifr.ifr_flags |=  (IFF_UP | IFF_RUNNING);
        if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) >= 0) {
            T_D("%s is UP", device_name);
        } else {
            T_E("ioctl(%s, SIOCSIFFLAGS) failed: %s", device_name, strerror(errno));
        }
    } else {
        T_W("ioctl(%s, SIOCGIFFLAGS) failed: %s", device_name, strerror(errno));
    }

    close(sockfd);
}

/****************************************************************************/
// CX_socket_open()
/****************************************************************************/
static int CX_socket_open(void)
{
    int                fd;
    struct ifreq       ifreq;
    struct sockaddr_ll addr;
    const char         *device_name = VTSS_NPI_DEVICE;

    // Make sure the NPI device is up.
    CX_socket_up(device_name);

    // Open raw socket (i.e. layer 2) to send and receive on.
    // The EtherType is VTSS_PROTO (0x8880), which - if running on
    // 1) the internal CPU - is an artificial header, stripped/inserted
    //    by the kernel module prior/after transmitting/receiving
    //    the frame to the switch, which - in the Tx case - will use the
    //    IFH (Internal Frame Header) inside of the frame to figure out
    //    what to do with it.
    // 2) an external CPU - is a header that is present on the wire
    //    as well. The connected switch will recognize this header
    //    and know that it carries an IFH inside and forward it
    //    accordingly (in the Tx case).
    if ((fd = socket(PF_PACKET, SOCK_RAW, htons(VTSS_PROTO))) < 0) {
        T_E("Unable to create raw socket for VTSS_PROTO protocol. Error: %s", strerror(errno));
        return fd;
    }

    // Bind to the #device_name device.
    // First find the interface index of the device we only know by name.
    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, device_name, sizeof(ifreq.ifr_name));
    if (ioctl(fd, SIOCGIFINDEX, &ifreq) < 0) {
        T_E("Unable to get interface index for %s. Error: %s", device_name, strerror(errno));
        goto do_exit;
    }

    T_I("Device number associated with %s is %d", device_name, ifreq.ifr_ifindex);

    // Interface index
    npi_socket_address.sll_ifindex = ifreq.ifr_ifindex;

    // MAC address length
    npi_socket_address.sll_halen = ETH_ALEN;

    // Don't care MAC address
    memcpy(npi_socket_address.sll_addr, npi_encap, 6);

    // Now just bind the socket to the interface
    memset(&addr, 0, sizeof(addr));
    addr.sll_family   = AF_PACKET;
    addr.sll_protocol = htons(VTSS_PROTO); // All frames received from #device_name carry VTSS_PROTO as EtherType.
    addr.sll_ifindex  = ifreq.ifr_ifindex;

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        T_E("bind() failed: %s", strerror(errno));
        goto do_exit;
    }

    T_I("Bound to interface %s", device_name);

    // Set the socket in promiscuous mode. This is needed particularly on a
    // 3rd party NIC (external CPU), so that it not only receives B/C frames and
    // frames directed to the NIC's MAC address.
    if (CX_socket_promisc_set(fd, ifreq.ifr_ifindex, device_name) != VTSS_RC_OK) {
        // Error message printed.
        goto do_exit;
    }

    if (CX_internal_cpu) {
        // We need to adjust the socket's receive buffer size, which is not
        // something that gets allocated, but a value that is compared against in
        // $(KERNEL)/net/packet/af_packet.c#packet_rcv() in order to judge whether
        // to discard this frame or keep it. It may be discarded if the default
        // receive buffer size (SK_RMEM_MAX (163840) is not adequate and compares
        // to the number and size of the uFDMA's kernel buffers (see second argument
        // to __build_skb() in vc3fdma.c).
        // The matter of the fact is that we know that the uFDMA has a fixed amount
        // of buffers, so that it cannot flood the kernel, because they are reused
        // after the kernel has handled them, so we cannot allocate more than what
        // the uFDMA has configured as the number of buffers anyway.
        (void)CX_socket_rcvbuf_set(fd, device_name);
    }

    // Success
    return fd;

do_exit:
    if (fd >= 0) {
        (void)close(fd);
    }

    // Failure
    return -1;
}

/****************************************************************************/
// RX_thread_packet_handle()
// On entry, the following fields of packet_dscr must be initialized:
//   #frm_ptr and #act_len and possibly #opaque.
/****************************************************************************/
static void RX_thread_packet_handle(packet_dscr_t *packet_dscr)
{
    mesa_packet_rx_meta_t meta;
    int                   hdr_len;

    if (CX_internal_cpu) {
        // The FDMA network interface does not include the FCS in the Tx
        // direction, but it does in the Rx, because it may contain Meta info on
        // some platforms.

        // Frame layout on internal CPU:
        // [ENCAP DMAC][ENCAP SMAC][0x8880][0x00][0x0x][IFH][DMAC][SMAC][ETYPE]...
        hdr_len = NPI_ENCAP_LEN + RX_ifh_size;

        // The meta structure contains info that can't be obtained from the IFH
        // alone, but must come from e.g. frame data
        memset(&meta, 0, sizeof(meta));

        meta.length = packet_dscr->act_len - hdr_len - 4; // Exclude FCS

        if (RX_fcs_data) {
            // On JR2, the FCS carries a.o. sFlow data.
            meta.fcs = CX_ntohl(&packet_dscr->frm_ptr[packet_dscr->act_len - 4]);
        }

        meta.etype = CX_ntohs(&packet_dscr->frm_ptr[hdr_len + ETYPE_POS]);

        if (mesa_packet_rx_hdr_decode(NULL, &meta, packet_dscr->frm_ptr + NPI_ENCAP_LEN, &packet_dscr->rx_info) != VTSS_RC_OK) {
            T_EG(TRACE_GRP_RX, "mesa_packet_rx_hdr_decode() failed. Dropping frame");
            T_EG_HEX(TRACE_GRP_RX, packet_dscr->frm_ptr, 100);
            goto do_exit;
        }
    } else {
        vtss_clear(meta);

        // Normal NIC cards do not include the FCS in either direction
        meta.length = packet_dscr->act_len;

        // Frame layout on external CPU:
        // [DMAC][SMAC][ETYPE]...
        hdr_len = 0;

        meta.etype = CX_ntohs(&packet_dscr->frm_ptr[ETYPE_POS]);
    }

    T_NG(TRACE_GRP_RX, "IFH Socket: Received %d bytes of which %u bytes are for Encap and Rx IFH, etype = 0x%04x, fcs = 0x%08x", packet_dscr->act_len, hdr_len, meta.etype, meta.fcs);

    // The VLAN tag in the frame must match the port's settings, so we discard
    // it if not.
    if (packet_dscr->rx_info.hints & MESA_PACKET_RX_HINTS_VLAN_TAG_MISMATCH) {
        T_WG(TRACE_GRP_RX, "Dropping frame due to VLAN tag mismatch (iport = %u, frame etype = 0x%04x)", packet_dscr->rx_info.port_no, meta.etype);
        goto do_exit;
    }

    // The chip doesn't strip the VLAN tag from the frame prior to presenting it to
    // the application, so let's do it for it (for legacy reasons, the application
    // expects the VLAN tag to be stripped).
    if (packet_dscr->rx_info.tag_type != MESA_TAG_TYPE_UNTAGGED) {
        u16 tci = CX_ntohs(packet_dscr->frm_ptr + hdr_len + ETYPE_POS + 2);

        packet_dscr->rx_info.stripped_tag.tpid = meta.etype;
        packet_dscr->rx_info.stripped_tag.pcp = (tci & 0xE000) >> 13;
        packet_dscr->rx_info.stripped_tag.dei = (tci & 0x1000) >> 12;
        packet_dscr->rx_info.stripped_tag.vid = (tci & 0x0FFF) >>  0;

        // Move DMAC + SMAC to new position four bytes further into the frame...
        memmove(&packet_dscr->frm_ptr[hdr_len + 4], &packet_dscr->frm_ptr[hdr_len], ETYPE_POS);

        // ...and pretend the header is four bytes longer and the length four bytes shorter (for further processing)
        hdr_len     += 4;
        meta.length -= 4;
    } else {
        // mesa_packet_rx_hdr_decode() sets info.tag.tpid to the frame's original
        // outer ethertype, which is wrong if the frame is untagged, so set it
        // to 0 to indicate exactly this condition.
        packet_dscr->rx_info.tag.tpid = 0;
    }

    packet_dscr->frm_ptr       += hdr_len;
    packet_dscr->act_len        = meta.length;
    packet_dscr->rx_info.length = meta.length;
    RX_dispatch_init(packet_dscr);
    return;

do_exit:
    // We gotta free the packet memory if an error occurred.
    RX_thread_frm_free(packet_dscr);
}

/******************************************************************************/
// RX_register_based()
/******************************************************************************/
static int RX_register_based(uint8_t *frm_ptr, int max_len, mesa_packet_rx_info_t &rx_info)
{
    if (mesa_packet_rx_frame(nullptr, frm_ptr, max_len, &rx_info) != MESA_RC_OK) {
        return 0;
    }

    return rx_info.length;
}

/****************************************************************************/
// RX_thread()
/****************************************************************************/
static void RX_thread(vtss_addrword_t data)
{
    packet_dscr_t packet_dscr;
    int           len, max_len;

    T_IG(TRACE_GRP_RX, "Starting packet Rx thread");

    msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_PRE, VTSS_MODULE_ID_PACKET);

    if (CX_internal_cpu) {
        // On the internal CPU, we need to open the NPI device and use that for
        // receiving and transmitting frames.
        if ((ifh_sock = CX_socket_open()) < 0) {
            return;
        }
    } else {
        // On an external CPU, we need to use register-based Rx/Tx.
    }

    packet_dscr.frm_ptr = NULL;

    while (1) {
        // RX_mtu is variable.
        max_len = RX_mtu + NPI_ENCAP_LEN + RX_ifh_size;

        // Wait until we're allowed to read new packets.
        // The reason that we need to limit the number of packets owned by the
        // application is that as soon as it has been recv()'d, the kernel can
        // reuse its buffer. So if we didn't limit ourselves, we could end up
        // using all the memory.
        vtss_sem_wait(&RX_packet_sem);

        // Attempt to allocate a buffer to receive frame into
        while (!packet_dscr.frm_ptr) {
            packet_dscr.frm_ptr = (u8 *)VTSS_MALLOC(max_len);
            if (!packet_dscr.frm_ptr) {
                // Out of memory. Wait a bit and try again.
                T_WG(TRACE_GRP_RX, "Out of memory. Sleeping one second");
                VTSS_OS_MSLEEP(1000);
            }
        }

        if (CX_internal_cpu) {
            if ((len = recv(ifh_sock, packet_dscr.frm_ptr, max_len, MSG_TRUNC)) <= 0) {
                T_EG(TRACE_GRP_RX, "recv() failed: %s", strerror(errno));
                vtss_sem_post(&RX_packet_sem);
                sleep(1);
                continue;
            }
        } else {
            if ((len = RX_register_based(packet_dscr.frm_ptr, max_len, packet_dscr.rx_info)) <= 0) {
                // No more frames. Wait 100 ms for next poll.
                vtss_sem_post(&RX_packet_sem);
                VTSS_OS_MSLEEP(100);
                continue;
            }
        }

        if (len > max_len) {
            // The MSG_TRUNC parameter to recv() causes it to return the number
            // of available bytes rather than the number of copied bytes.
            // This allows us to detect that the kernel's Rx MTU is larger than
            // our own. This should only be a temporary condition until we get
            // a new packet allocated.
            T_I("Discarding oversize frame (len = %u, max_len = %u)", len, max_len);
            VTSS_FREE(packet_dscr.frm_ptr);
            packet_dscr.frm_ptr = NULL;
            vtss_sem_post(&RX_packet_sem);
            continue;
        }

        // Hand ownership of frame buffer to this function
        packet_dscr.act_len = len;

        // Save a copy of the frame pointer, we allocated, so that we can free it again,
        // because RX_thread_packet_handle() is allowed to change all fields in #packet_dscr
        // except for #opaque.
        packet_dscr.opaque = packet_dscr.frm_ptr;

        (void)vtss_mutex_lock(&RX_low_level_mutex);

        RX_total++;

        if (++RX_outstanding > RX_outstanding_max) {
            RX_outstanding_max = RX_outstanding;
        }

        vtss_mutex_unlock(&RX_low_level_mutex);

        if (RX_processing) {
            RX_thread_packet_handle(&packet_dscr);
        } else {
            RX_thread_frm_free(&packet_dscr);
        }

        // When we get here, we may use packet_dscr (and therefore packet_dscr.opaque and packet_dscr.frm_ptr) again.
        packet_dscr.frm_ptr = NULL;
    }

    T_EG(TRACE_GRP_RX, "UNREACHABLE");
}

/****************************************************************************/
// DBG_cmd_stat_module_print()
// cmd_text   : "Print per-module statistics",
// arg_syntax : NULL,
// max_arg_cnt: 0
/****************************************************************************/
static void DBG_cmd_stat_module_print(packet_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    u32 a, f, total;
    int i;

    // Statistics
    (void)dbg_printf("\nModule                Rx Pkts    Rx Bytes     Tx Pkts    Tx Bytes     Max Rx Callback [ms]\n");
    (void)dbg_printf(  "--------------------- ---------- ------------ ---------- ------------ --------------------\n");

    PACKET_RX_FILTER_CRIT_ENTER();

    for (i = 0; i <= VTSS_MODULE_ID_NONE; i++) {
        packet_module_counters_t *cntrs = &CX_module_counters[i];
        if (cntrs->rx_pkts != 0 || cntrs->tx_pkts != 0) {
            (void)dbg_printf("%-21s %10u " VPRI64Fu("12")" %10u " VPRI64Fu("12")" " VPRI64Fu("20")"\n", vtss_module_names[i], cntrs->rx_pkts, cntrs->rx_bytes, cntrs->tx_pkts, cntrs->tx_bytes, VTSS_OS_TICK2MSEC(cntrs->longest_rx_callback_ticks));
        }
    }
    (void)dbg_printf("%-21s %10u " VPRI64Fu("12")"\n\n", "<no subscriber>", RX_pkts_no_subscribers, RX_bytes_no_subscribers);

    PACKET_RX_FILTER_CRIT_EXIT();

    PACKET_CX_COUNTER_CRIT_ENTER();
    a = TX_alloc_calls;
    f = TX_free_calls;
    PACKET_CX_COUNTER_CRIT_EXIT();

    (void)vtss_mutex_lock(&RX_low_level_mutex);
    total = RX_total;
    vtss_mutex_unlock(&RX_low_level_mutex);

    (void)dbg_printf("Rx Processing: %s\n", RX_processing ? "Enabled" : "Disabled");
    (void)dbg_printf("Rx Total     : %10u\n\n", total);

    (void)dbg_printf("Tx packet buffers\n");
    (void)dbg_printf("-----------------\n");
    (void)dbg_printf("Allocations  : %10u\n", a);
    (void)dbg_printf("Deallocations: %10u\n", f);
    (void)dbg_printf("Outstanding  : %10u\n\n", a - f);
}

/****************************************************************************/
// DBG_cmd_stat_thread_print()
// cmd_text   : "Print per-thread statistics",
// arg_syntax : NULL,
// max_arg_cnt: 0
/****************************************************************************/
static void DBG_cmd_stat_thread_print(packet_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    u32 a, b;
    int thread_prio;

    // Statistics
    (void)dbg_printf("\nThread Priority Rx Pkts    Rx Bytes     Max Rx Callback [ms] FIFO Max FIFO Cur FIFO Sz FIFO Over FIFO Under\n");
    (void)dbg_printf(  "--------------- ---------- ------------ -------------------- -------- -------- ------- --------- ----------\n");

    PACKET_RX_DISPATCH_THREAD_CRIT_ENTER();
    for (thread_prio = 0; thread_prio < ARRSZ(RX_dispatch_thread_states); thread_prio++) {
        packet_rx_dispatch_thread_state_t *thread_state = &RX_dispatch_thread_states[thread_prio];
        u32                               m, t, c, s, o, u;

        if (!thread_state->thread_handle) {
            // Thread not created (yet)
            continue;
        }

        // Args are: max_cnt, total_cnt, cur_cnt, cur_sz, overruns, underruns
        vtss_fifo_cp_get_statistics(&thread_state->fifo, &m, &t, &c, &s, &o, &u);

        (void)dbg_printf("%-15s %10u " VPRI64Fu("12") " " VPRI64Fu("20") " %8u %8u %7u %9u %10u\n",
                         vtss_thread_prio_to_txt((vtss_thread_prio_t)thread_prio),
                         thread_state->rx_pkts, thread_state->rx_bytes,
                         VTSS_OS_TICK2MSEC(thread_state->longest_rx_callback_ticks),
                         m, c, s, o, u);
    }
    PACKET_RX_DISPATCH_THREAD_CRIT_EXIT();

    PACKET_RX_FILTER_CRIT_ENTER();
    a = RX_filter_lists_alive;
    b = RX_filter_lists_total;
    PACKET_RX_FILTER_CRIT_EXIT();

    (void)dbg_printf("\nRx Filter Lists\n");
    (void)dbg_printf("---------------\n");
    (void)dbg_printf("Alive: %8u\n", a);
    (void)dbg_printf("Total: %8u\n\n", b);

    (void)vtss_mutex_lock(&RX_low_level_mutex);
    a = RX_outstanding;
    b = RX_outstanding_max;
    vtss_mutex_unlock(&RX_low_level_mutex);

    (void)dbg_printf("Rx In Progress\n");
    (void)dbg_printf("--------------\n");
    (void)dbg_printf("Now: %9u\n", a);
    (void)dbg_printf("Max: %9u\n\n", b);
}

/****************************************************************************/
// DBG_cmd_stat_rx_qu_print()
// cmd_text   : "Print Rx queue statistics",
// arg_syntax : NULL,
// max_arg_cnt: 0
/****************************************************************************/
static void DBG_cmd_stat_rx_qu_print(packet_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    int i;
    uint64_t rx_qu_counters[8];

    PACKET_CX_COUNTER_CRIT_ENTER();
    memcpy(rx_qu_counters, RX_xtr_qu_counters, sizeof(rx_qu_counters));
    PACKET_CX_COUNTER_CRIT_EXIT();

    (void)dbg_printf("Qu Frames\n");
    (void)dbg_printf("-- --------------\n");

    for (i = 0; i < ARRSZ(rx_qu_counters); i++) {
        dbg_printf("%2u " VPRI64Fu("14") "\n", i, rx_qu_counters[i]);
    }
}

/****************************************************************************/
// DBG_cmd_stat_fdma_print()
// cmd_text   : "Print FDMA statistics"
// arg_syntax : NULL,
// max_arg_cnt: 0
/****************************************************************************/
static void DBG_cmd_stat_fdma_print(packet_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    (void)dbg_printf("Obsolete. Use 'debug ufdma'\n");
}

/****************************************************************************/
// DBG_cmd_stat_port_print()
// cmd_text   : "Print port statistics"
// arg_syntax : NULL
// max_arg_cnt: 0
/****************************************************************************/
static void DBG_cmd_stat_port_print(packet_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    int iport, prio;

    // Port Statistics
    (void)dbg_printf("\nPort      ");
    for (prio = 0; prio < VTSS_PRIOS; prio++) {
        (void)dbg_printf("Rx Prio %d  ", prio);
    }
    (void)dbg_printf("Rx Super   Tx\n");

    (void)dbg_printf("--------- ");
    for (prio = 0; prio <= VTSS_PRIOS + 1; prio++) {
        (void)dbg_printf("---------- ");
    }
    (void)dbg_printf("\n");

    PACKET_CX_COUNTER_CRIT_ENTER();
    for (iport = 0; iport <= CX_port_cnt; iport++) {
        u32 idx = iport;
        if (iport == CX_port_cnt) {
            (void)dbg_printf("Unknown   "); // Typically sFlow frames.
        } else {
            (void)dbg_printf("%9u ", iport2uport(iport));
        }
        for (prio = 0; prio <= VTSS_PRIOS; prio++) {
            (void)dbg_printf("%10u ", CX_port_counters.rx_pkts[idx][prio]);
        }
        if (iport == CX_port_cnt) {
            (void)dbg_printf("%10s ", "N/A");
        } else {
            (void)dbg_printf("%10u ", CX_port_counters.tx_pkts[iport]);
        }
        (void)dbg_printf("\n");
    }
    (void)dbg_printf("Switched  ");
    for (prio = 0; prio <= VTSS_PRIOS; prio++) {
        (void)dbg_printf("%10s ", "N/A");
    }
    (void)dbg_printf("%10u ", CX_port_counters.tx_pkts[CX_port_cnt]);
    (void)dbg_printf("\nMulticast ");
    for (prio = 0; prio <= VTSS_PRIOS; prio++) {
        (void)dbg_printf("%10s ", "N/A");
    }
    (void)dbg_printf("%10u ", CX_port_counters.tx_pkts[CX_port_cnt + 1]);
    (void)dbg_printf("\n");
    PACKET_CX_COUNTER_CRIT_EXIT();
}

/******************************************************************************/
// RX_subscriber_match_str()
/******************************************************************************/
char *RX_subscriber_match_str(char *buf, size_t size, u32 match)
{
    int cnt = 0;

    if (!buf || size == 0) {
        return NULL;
    }

#define RX_MATCH_STR(_str_) do {cnt += snprintf(buf + cnt, MAX((int)size - cnt, 0), "%s" vtss_xstr(_str_), cnt != 0 ? ", " : "");} while (0);

    buf[0] = '\0';

    if (match == 0) {
        RX_MATCH_STR(Any);
    }
    if (match & PACKET_RX_FILTER_MATCH_SRC_PORT) {
        RX_MATCH_STR(Ingress Port);
    }
    if (match & PACKET_RX_FILTER_MATCH_ACL) {
        RX_MATCH_STR(ACL);
    }
    if (match & PACKET_RX_FILTER_MATCH_VID) {
        RX_MATCH_STR(VID);
    }
    if (match & PACKET_RX_FILTER_MATCH_DMAC) {
        RX_MATCH_STR(DMAC);
    }
    if (match & PACKET_RX_FILTER_MATCH_SMAC) {
        RX_MATCH_STR(SMAC);
    }
    if (match & PACKET_RX_FILTER_MATCH_ETYPE) {
        RX_MATCH_STR(EtherType);
    }
    if (match & PACKET_RX_FILTER_MATCH_IP_PROTO) {
        RX_MATCH_STR(IP Protocol);
    }
    if (match & PACKET_RX_FILTER_MATCH_SSPID) {
        RX_MATCH_STR(SSPID);
    }
    if (match & PACKET_RX_FILTER_MATCH_UDP_SRC_PORT) {
        RX_MATCH_STR(UDP Src. Port);
    }
    if (match & PACKET_RX_FILTER_MATCH_UDP_DST_PORT) {
        RX_MATCH_STR(UDP Dest. Port);
    }
    if (match & PACKET_RX_FILTER_MATCH_TCP_SRC_PORT) {
        RX_MATCH_STR(TCP Src. Port);
    }
    if (match & PACKET_RX_FILTER_MATCH_TCP_DST_PORT) {
        RX_MATCH_STR(TCP Dest. Port);
    }
    if (match & PACKET_RX_FILTER_MATCH_SFLOW) {
        RX_MATCH_STR(sFlow);
    }
    if (match & PACKET_RX_FILTER_MATCH_IP_ANY) {
        RX_MATCH_STR(IPv4 + IPv6 + ARP);
    }
    if (match & PACKET_RX_FILTER_MATCH_DEFAULT) {
        RX_MATCH_STR(Default);
    }

#undef RX_MATCH_STR

    if (cnt == 0) {
        (void)snprintf(buf, size, "<none>");
    }
    return buf;
}

/****************************************************************************/
// DBG_cmd_subscribers_print()
// cmd_text   : "Print subscriber list"
// arg_syntax : NULL
// max_arg_cnt: 0
/****************************************************************************/
static void DBG_cmd_subscribers_print(packet_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    packet_rx_filter_item_t *l;
    int cnt = 0;

    (void)dbg_printf("\nModule                Priority   Match\n");
    (void)dbg_printf(  "--------------------- ---------- ---------------------------------------------\n");

    PACKET_RX_FILTER_CRIT_ENTER();
    l = RX_filter_list;
    while (l) {
        char buffer[200];
        (void)dbg_printf("%-21s %10u %s\n", vtss_module_names[l->filter.modid], l->filter.prio, RX_subscriber_match_str(buffer, sizeof(buffer), l->filter.match));
        cnt++;
        l = l->next;
    }

    if (cnt == 0) {
        (void)dbg_printf("<none>\n");
    }

    PACKET_RX_FILTER_CRIT_EXIT();
}

/****************************************************************************/
// DBG_cmd_stat_packet_clear()
// cmd_text   : "Clear per-module statistics",
// arg_syntax : NULL,
// max_arg_cnt: 0
/****************************************************************************/
static void DBG_cmd_stat_packet_clear(packet_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    // Clear Rx Statistics
    PACKET_RX_FILTER_CRIT_ENTER();
    memset(CX_module_counters, 0, sizeof(CX_module_counters));
    RX_pkts_no_subscribers  = 0;
    RX_bytes_no_subscribers = 0;
    PACKET_RX_FILTER_CRIT_EXIT();

    (void)vtss_mutex_lock(&RX_low_level_mutex);
    RX_total = 0;
    vtss_mutex_unlock(&RX_low_level_mutex);

    (void)dbg_printf("Module statistics cleared!\n");
}

/****************************************************************************/
// DBG_cmd_stat_port_clear()
// cmd_text   : "Clear port statistics",
// arg_syntax : NULL,
// max_arg_cnt: 0
/****************************************************************************/
static void DBG_cmd_stat_port_clear(packet_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    PACKET_CX_COUNTER_CRIT_ENTER();
    vtss_clear(CX_port_counters);
    PACKET_CX_COUNTER_CRIT_EXIT();
    (void)dbg_printf("Port statistics cleared!\n");
}

/****************************************************************************/
// DBG_cmd_stat_thread_clear()
// cmd_text   : "Clear per-thread statistics",
// arg_syntax : NULL,
// max_arg_cnt: 0
/****************************************************************************/
static void DBG_cmd_stat_thread_clear(packet_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    int thread_prio;

    // Clear per-thread Statistics
    PACKET_RX_DISPATCH_THREAD_CRIT_ENTER();
    for (thread_prio = 0; thread_prio < ARRSZ(RX_dispatch_thread_states); thread_prio++) {
        packet_rx_dispatch_thread_state_t *thread_state = &RX_dispatch_thread_states[thread_prio];

        if (!thread_state->thread_handle) {
            // Thread not created (yet)
            continue;
        }

        thread_state->rx_bytes = 0;
        thread_state->rx_pkts  = 0;
        thread_state->longest_rx_callback_ticks = 0;
        vtss_fifo_cp_clr_statistics(&thread_state->fifo);
    }

    PACKET_RX_DISPATCH_THREAD_CRIT_EXIT();

    (void)vtss_mutex_lock(&RX_low_level_mutex);
    // Cannot clear RX_outstanding, because that will cause the counter to
    // become negative when RX_thread_frm_free() is invoked
    // (which it will be if RX_outstanding is > 0 in this function).
    RX_outstanding_max = 0;
    vtss_mutex_unlock(&RX_low_level_mutex);

    (void)dbg_printf("Rx thread statistics cleared!\n");
}

/****************************************************************************/
// DBG_cmd_stat_rx_qu_clear()
// cmd_text   : "Clear Rx queue statistics",
// arg_syntax : NULL,
// max_arg_cnt: 0
/****************************************************************************/
static void DBG_cmd_stat_rx_qu_clear(packet_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    PACKET_CX_COUNTER_CRIT_ENTER();
    vtss_clear(RX_xtr_qu_counters);
    PACKET_CX_COUNTER_CRIT_EXIT();
}

static void CX_ufdma_stati_clear(void);

/****************************************************************************/
// DBG_cmd_stat_all_clear()
// cmd_text   : "Clear all statistics",
// arg_syntax : NULL,
// max_arg_cnt: 0
/****************************************************************************/
static void DBG_cmd_stat_all_clear(packet_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    DBG_cmd_stat_packet_clear(dbg_printf, parms_cnt, parms);
    DBG_cmd_stat_port_clear(dbg_printf, parms_cnt, parms);
    DBG_cmd_stat_thread_clear(dbg_printf, parms_cnt, parms);
    DBG_cmd_stat_rx_qu_clear(dbg_printf, parms_cnt, parms);
    CX_ufdma_stati_clear();
}

/****************************************************************************/
// DBG_cmd_cfg_stack_trace()
// cmd_text   : "Enable or disable stack trace"
// arg_syntax : "0: Disable, 1: Enable"
// max_arg_cnt: 1
/****************************************************************************/
static void DBG_cmd_cfg_stack_trace(packet_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    if (parms_cnt == 0) {
        (void)dbg_printf("Stack trace is currently %sabled\n", CX_stack_trace_ena ? "en" : "dis");
    } else if (parms_cnt == 1) {
        CX_stack_trace_ena = parms[0] != 0;
    } else {
        DBG_cmd_syntax_error(dbg_printf, "This function takes 1 parameter, which must be 0 or 1");
    }
}

/****************************************************************************/
// DBG_cmd_test_syslog()
// cmd_text   : "Generate error or fatal. This is only to test the SYSLOG, and has nothing to do with the packet module"
// arg_syntax : "0: Generate error, 1: Generate assert (never returns)"
// max_arg_cnt: 1
/****************************************************************************/
static void DBG_cmd_test_syslog(packet_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    static int err_cnt = 0;

    /*lint -esym(459, DBG_cmd_test_syslog) unprotected access to err_cnt. Fine */
    if (parms_cnt == 1 && parms[0] == 0) {
        // Generate error
        T_E("Test Error #%d", ++err_cnt);
    } else if (parms_cnt == 1 && parms[0] == 1) {
        T_E("Generating Assertion");
        VTSS_ASSERT(FALSE);
    } else {
        DBG_cmd_syntax_error(dbg_printf, "This function takes 1 parameter, which must be 0 or 1");
    }
}

/****************************************************************************/
/****************************************************************************/
static void TX_init(void)
{
    TX_insert_tag = fast_cap(MESA_CAP_PACKET_INS_TAG);
    TX_masq_port  = fast_cap(MESA_CAP_PACKET_MASQ_PORT);
    TX_ptp_action = fast_cap(MESA_CAP_PACKET_PTP_ACTION);
}

/****************************************************************************/
// Initialize DSR->Thread FIFO and mutex/condition variables used in that
// synchronization, together with the thread itself.
/****************************************************************************/
static void RX_init(void)
{
    RX_ifh_size = fast_cap(MESA_CAP_PACKET_RX_IFH_SIZE);
    RX_fcs_data = fast_cap(MESA_CAP_PACKET_FCS_DATA);

    // In order to be able to route jumbo frames that have no ARP entry, we must
    // increase the FDMA's Rx MTU to the desired maximum size.
    // Notice that if the IP jumbo frame's "Don't Fragment" (DF) bit is set,
    // this will not help us, because the IP stack's MTU is still 1500 bytes, so
    // it cannot send out an IP frame larger than that and if DF is set, it will
    // result in an ICMP frame back to the sender.
    RX_mtu = fast_cap(MESA_CAP_PORT_FRAME_LENGTH_MAX);

    // The following is only applicable on internal MIPS CPUs.
    // Configure the FDMA with this MTU.
    if (CX_internal_mips) {
        packet_rx_cfg_t rx_cfg;
        mesa_rc         rc;

        if ((rc = packet_rx_cfg_get(&rx_cfg)) != VTSS_RC_OK) {
            T_E("Unable to get Rx config from kernel (%s)", error_txt(rc));
            memset(&rx_cfg, 0, sizeof(rx_cfg));
        }

        rx_cfg.mtu = RX_mtu;

        if ((rc = packet_rx_cfg_set(&rx_cfg)) != VTSS_RC_OK) {
            T_E("Unable to set Rx config in kernel (%s)", error_txt(rc));
        }
    }

    // Counting semaphore that limits the number of simultaneously, unprocessed, received frames.
    vtss_sem_init(&RX_packet_sem, RX_BUF_CNT);

    // Only used for counting at the lowest level.
    vtss_mutex_init(&RX_low_level_mutex);

    // Critical region protecting the packet subscription filter list.
    critd_init(&RX_filter_crit, "packet_rx_filter", VTSS_MODULE_ID_PACKET, CRITD_TYPE_MUTEX);

    // Critical region protecting the FIFO containing frames to be dispatched from a particular priority
    critd_init(&RX_dispatch_thread_crit, "packet_rx_dispatch_thread", VTSS_MODULE_ID_PACKET, CRITD_TYPE_MUTEX);

    // Mutex protecting counter updates
    critd_init(&CX_counter_crit, "packet_counters", VTSS_MODULE_ID_PACKET, CRITD_TYPE_MUTEX);

    // Do similar things for IPv6
    // Defaults in the Linux kernel are:
    // Low  = 3 MBytes
    // High = 4 MBytes
    // Time = 60 seconds, hence adjust to two seconds here.
    //
    // There does not seem to be any significant difference in
    // behavior whether ipfrag_time is set to 1 or left at default
    // 60. Therefore, revert to default value

#ifdef VTSS_SW_OPTION_IPV6
    CX_sysctl_set("/proc/sys/net/ipv6/ip6frag_low_thresh", 65536);
    CX_sysctl_set("/proc/sys/net/ipv6/ip6frag_high_thresh", 131072);
#endif

    // Create packet thread. This is the thread on which the frames are received
    // from the kernel. The frames are then dispatched to other threads, depending
    // on user-modules priority for its filters. This initial thread must have a
    // very high priority, since it must run in case a frame comes in that matches
    // a high user-module requested priority.
    vtss_thread_create(VTSS_THREAD_PRIO_HIGHEST,
                       RX_thread,
                       0,
                       "Packet RX",
                       nullptr,
                       0,
                       &RX_thread_handle,
                       &RX_thread_state);
}

/****************************************************************************/
// RX_s_custom_etype_change_hook()
/****************************************************************************/
static void RX_s_custom_etype_change_hook(void)
{
    mesa_rc      rc;
    mesa_etype_t tpid;

    // Get notified if the S-custom VLAN tag's TPID changes
    vlan_s_custom_etype_change_register(VTSS_MODULE_ID_PACKET, RX_vlan_s_custom_etype_change_callback);

    // And update our own cached version.
    if ((rc = vtss_appl_vlan_s_custom_etype_get(&tpid)) != VTSS_RC_OK) {
        tpid = 0x88a8;
        T_W("vtss_appl_vlan_s_custom_etype_get() failed (%s). Setting to 0x88a8", error_txt(rc));
    }

    RX_vlan_s_custom_etype_change_callback(tpid);
}

/****************************************************************************/
// RX_conf_setup()
/****************************************************************************/
static void RX_conf_setup(void)
{
    mesa_packet_rx_conf_t rx_conf;

    // mesa_packet_rx_conf_get()/set() must be called without interference.
    VTSS_APPL_API_LOCK_SCOPE();

    // Get Rx packet configuration */
    PACKET_CHECK(mesa_packet_rx_conf_get(0, &rx_conf) == VTSS_RC_OK, return;);

    // Setup Rx queue mapping */
    rx_conf.map.bpdu_queue      = PACKET_XTR_QU_BPDU;
    rx_conf.map.garp_queue      = PACKET_XTR_QU_BPDU;
    rx_conf.map.learn_queue     = PACKET_XTR_QU_LEARN;
    rx_conf.map.igmp_queue      = PACKET_XTR_QU_IGMP;
    rx_conf.map.ipmc_ctrl_queue = PACKET_XTR_QU_IGMP;
    rx_conf.map.mac_vid_queue   = PACKET_XTR_QU_MAC;
    rx_conf.map.lrn_all_queue   = PACKET_XTR_QU_LRN_ALL;
#if defined(VTSS_SW_OPTION_SFLOW)
    rx_conf.map.sflow_queue     = PACKET_XTR_QU_SFLOW;
#else
    // Do not change the sflow_queue.
#endif /* VTSS_SW_OPTION_SFLOW */

    rx_conf.map.l3_uc_queue     = PACKET_XTR_QU_MGMT_MAC;
    rx_conf.map.l3_other_queue  = PACKET_XTR_QU_L3_OTHER;

    // RedBox-related frames
    rx_conf.map.sv_queue        = PACKET_XTR_QU_REDBOX;

    // Setup CPU queue sizes.
    rx_conf.queue[PACKET_XTR_QU_LOWEST  - VTSS_PACKET_RX_QUEUE_START].size =   8 * 1024;
    rx_conf.queue[PACKET_XTR_QU_LOWER   - VTSS_PACKET_RX_QUEUE_START].size =   8 * 1024;
    rx_conf.queue[PACKET_XTR_QU_LOW     - VTSS_PACKET_RX_QUEUE_START].size =   8 * 1024;
    rx_conf.queue[PACKET_XTR_QU_NORMAL  - VTSS_PACKET_RX_QUEUE_START].size =   8 * 1024;
    rx_conf.queue[PACKET_XTR_QU_MEDIUM  - VTSS_PACKET_RX_QUEUE_START].size =  16 * 1024;
    rx_conf.queue[PACKET_XTR_QU_HIGH    - VTSS_PACKET_RX_QUEUE_START].size =   8 * 1024;
    rx_conf.queue[PACKET_XTR_QU_HIGHER  - VTSS_PACKET_RX_QUEUE_START].size =  12 * 1024;
    rx_conf.queue[PACKET_XTR_QU_HIGHEST - VTSS_PACKET_RX_QUEUE_START].size =   8 * 1024;

    for (int i = 0; i < /*array_size(rx_conf.queue)*/8; i++) {
        rx_conf.queue[i].npi.enable = true;
    }

    // Set Rx packet configuration */
    PACKET_CHECK(mesa_packet_rx_conf_set(0, &rx_conf) == VTSS_RC_OK, return;);
}

/****************************************************************************/
// CX_netlink_channel_id_get()
/****************************************************************************/
static int CX_netlink_channel_id_get(void)
{
    static int        id = 0;
    static const char *netlink_name = "vtss_packet";

    if (id != 0 && id != -1) {
        return id;
    }

    id = vtss::appl::netlink::genelink_channel_by_name(netlink_name, __FUNCTION__);
    if (id == -1) {
        T_EG(TRACE_GRP_NETLINK, "Failed to get netlink channel for %s", netlink_name);
    }

    return id;
}

/****************************************************************************/
// RX_proc_sys_net_ipv4_write()
/****************************************************************************/
static void RX_proc_sys_net_ipv4_write(const char *file, const char *val)
{
    int  fd;
    char path[200];

    snprintf(path, sizeof(path), "/proc/sys/net/ipv4/%s", file);
    path[sizeof(path) - 1] = '\0';

    T_IG(TRACE_GRP_RX, "Writing %s to %s", val, path);

    if ((fd = open(path, O_RDWR)) < 0) {
        T_EG(TRACE_GRP_RX, "Unable to open \"%s\". Error = %s", path, strerror(errno));
    } else {
        if (write(fd, val, strlen(val)) < 0) {
            T_EG(TRACE_GRP_RX, "Unable to write to \"%s\". Error = %s", path, strerror(errno));
        }

        (void)close(fd);
    }
}

/****************************************************************************/
// RX_tcp_rx_win_set()
/****************************************************************************/
static void RX_tcp_rx_win_set(void)
{
    // Set TCP Rx Window sizes we can cope with, or we might drop IP frames
    // in the queue system, which in turn causes TCP retransmissions once
    // the TCP protocol discovers it. This is mainly a problem when downloading
    // large files to the switch (like firmware images).
    //
    // Linux' IP stack contains two values of importance here. They are both
    // initialized during boot, and their values depend on the amount of RAM
    // available, so the advertized receive window may differ from platform
    // to platform unless we do something here.
    //
    // The two values are "tcp_rmem" (vector of three values, <min, def, max>)
    // and "tcp_window_scaling".
    //
    // In order to judge the values to use, one would normally multiply the
    // bestcase and worstcast round-trip-times with the link speed to obtain
    // so-called Bandwidth-Delay Products, but in our case, this is not good
    // enough, because we don't have a CPU with infinite power, so we cannot
    // keep up with the stream of frames coming in. As said, this may lead to
    // the H/W queue system dropping frames, and in turn cause retransmissions.
    //
    // So in fact, the values to use depend - among others - at least on these
    // things:
    //  1) The CPU's performance,
    //  2) the Linux stack's performance,
    //  3) the number of Rx buffers configured in the uFDMA.
    //  4) the configuration of the Rx throttling of the IP management queue.
    //
    // Finding optimal values for the two parameters is pretty tough, but
    // experiments (rather than analysis) have shown that we can keep up the
    // pace if setting tcp_window_scaling to 0 and leave the tcp_rmem vector
    // as is. This will give a transfer time at around 10 secs for a 10 Mbyte
    // file. We could probably get down to a couple or three seconds by tweeking
    // further.
    // Setting tcp_window_scaling to 0 will ensure that we can't get more than
    // 64 Kbytes worth of TCP data from the transmitting end without
    // acknowledgements. This translates to around 45 max-sized Ethernet frames,
    // so the number of uFDMA Rx buffers must be > 45 * max-number-of-tcp-
    // -connections-on-which-we-can-receive-large-amounts-of-data-simultaneously
    // (if throttling doesn't kick in first).
    RX_proc_sys_net_ipv4_write("tcp_window_scaling", "0");

    // This is how you would update the tcp_rmem vector if needed (on Serval-1
    // it defaults to "4096 87380 933856"):
    // RX_proc_sys_net_ipv4_write("tcp_rmem", "4096 40000 130000");

    // You can also try from a shell:
    //    echo "4096 40000 130000" > /proc/sys/net/ipv4/tcp_rmem
}

/****************************************************************************/
// CX_netlink_req class.
/****************************************************************************/
class CX_netlink_req
{
public:
    // Constructor
    CX_netlink_req(int cmd)
    {
        netlink_msg_hdr.nlmsg_seq = vtss::appl::netlink::netlink_seq();
        netlink_msg_hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct genlmsghdr));
        netlink_msg_hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
        netlink_msg_hdr.nlmsg_type = CX_netlink_channel_id_get();
        generic_netlink_msg_hdr.cmd = cmd;
        generic_netlink_msg_hdr.version = 0;
    }

    // THE ORDER OF THE FOLLOWING MEMBERS IS IMPORTANT!
    // This structure is being casted to a "struct nlmsghdr" plus payload...
    static constexpr u32 max_size_bytes = 1000;
    struct nlmsghdr netlink_msg_hdr = {};
    struct genlmsghdr generic_netlink_msg_hdr = {};
    char attr[max_size_bytes];
};

/******************************************************************************/
// CX_netlink_rx_throttle_cfg_get class
// Contains a callback function invoked when current throttle config
// is retrieved from the kernel.
/******************************************************************************/
class CX_netlink_rx_throttle_cfg_get : public vtss::appl::netlink::NetlinkCallbackAbstract
{
public:
    void operator()(struct sockaddr_nl *addr, struct nlmsghdr *netlink_msg_hdr)
    {
        struct genlmsghdr *genl;
        int               len;
        struct rtattr     *attr;
        bool              queues_seen[ARRSZ(throttle_cfg.frm_limit_per_tick)], period_seen = false;
        u32               qu;

        genl = (struct genlmsghdr *)NLMSG_DATA(netlink_msg_hdr);
        if ((len  = netlink_msg_hdr->nlmsg_len - NLMSG_LENGTH(sizeof(*genl))) < 0) {
            T_EG(TRACE_GRP_NETLINK, "Msg too short for this type (%d)!", len);
            return;
        }

        memset(&throttle_cfg, 0, sizeof(throttle_cfg));
        memset(queues_seen, 0, sizeof(queues_seen));

        attr = GENL_RTA(genl);

        T_DG(TRACE_GRP_NETLINK, "Whole payload (starts @ %p)", attr);
        T_DG_HEX(TRACE_GRP_NETLINK, (const uchar *)attr, len);

        // An attribute consists of:
        // attr->rta_len:  a 2 byte length, which includes itself, attr->rta_type, and the data following it.
        // attr->rta_type: a 2 byte attribute index (e.g. VTSS_PACKET_ATTR_RX_THROTTLE_TICK_PERIOD_MSEC). Two MSbits are special (indicate nesting and byte order).
        // Payload:        RTA_PAYLOAD(attr) bytes of data forming the value of the attribute.

        while (RTA_OK(attr, len)) {
            T_DG(TRACE_GRP_NETLINK, "RTA_PAYLOAD(attr) = %u, attr->rta_len = %u. Dumping attr->rta_len bytes from %p, which is attr ID = 0x%x", RTA_PAYLOAD(attr), attr->rta_len, attr, attr->rta_type);
            T_DG_HEX(TRACE_GRP_NETLINK, (const uchar *)attr, attr->rta_len);

            // Filter out two MSbits with NLA_TYPE_MASK
            switch (attr->rta_type & NLA_TYPE_MASK) {
            case VTSS_PACKET_ATTR_RX_THROTTLE_TICK_PERIOD_MSEC:
                if (RTA_PAYLOAD(attr) != 4) {
                    T_EG(TRACE_GRP_NETLINK, "Expected 4, got %u bytes", RTA_PAYLOAD(attr));
                    return;
                }

                // Not stored anywhere
                T_IG(TRACE_GRP_NETLINK, "Throttle period = %u ms", *(u32 *)RTA_DATA(attr));
                period_seen = true;
                break;

            case VTSS_PACKET_ATTR_RX_THROTTLE_QU_CFG: {
                struct rtattr *qu_attrs[VTSS_PACKET_ATTR_CNT];

                if (vtss::appl::netlink::parse_nested_attr(qu_attrs, VTSS_PACKET_ATTR_END, attr) != 0) {
                    T_EG(TRACE_GRP_NETLINK, "Unable to parse nested attribute for queue config");
                    return;
                }

                for (int i = 0; i < VTSS_PACKET_ATTR_CNT; i++) {
                    T_DG(TRACE_GRP_NETLINK, "qu_attrs[%d] = %p", i, qu_attrs[i]);
                }

                // In a get(), all parameters must be there, so the following macros complain if an attribute is not there.
                CX_NETLINK_ATTR_IDX_U32_GET(qu_attrs, VTSS_PACKET_ATTR_RX_THROTTLE_QU_NUMBER, qu);

                if (qu >= ARRSZ(throttle_cfg.frm_limit_per_tick)) {
                    T_EG(TRACE_GRP_NETLINK, "Got queue number %u, but only %u queues are configurable", qu, ARRSZ(throttle_cfg.frm_limit_per_tick));
                    return;
                }

                if (queues_seen[qu]) {
                    T_EG(TRACE_GRP_NETLINK, "Already got data for qu = %u", qu);
                    return;
                }

                queues_seen[qu] = true;

                CX_NETLINK_ATTR_IDX_U32_GET(qu_attrs, VTSS_PACKET_ATTR_RX_THROTTLE_FRM_LIMIT_PER_TICK,  throttle_cfg.frm_limit_per_tick[qu]);
                CX_NETLINK_ATTR_IDX_U32_GET(qu_attrs, VTSS_PACKET_ATTR_RX_THROTTLE_BYTE_LIMIT_PER_TICK, throttle_cfg.byte_limit_per_tick[qu]);
                CX_NETLINK_ATTR_IDX_U32_GET(qu_attrs, VTSS_PACKET_ATTR_RX_THROTTLE_SUSPEND_TICK_CNT,    throttle_cfg.suspend_tick_cnt[qu]);
                break;
            }

            default:
                T_EG(TRACE_GRP_NETLINK, "Unknown attribute 0x%x", attr->rta_type);
                return;
            }

            attr = RTA_NEXT(attr, len);
        }

        ok = true;

        // Final checks.

        // See if we got a VTSS_PACKET_ATTR_RX_THROTTLE_TICK_PERIOD_MSEC attribute
        if (!period_seen) {
            T_EG(TRACE_GRP_NETLINK, "Didn't get a tick period attribute");
            ok = false;
            // Keep going.
        }

        // See if we got data for all queues.
        for (qu = 0; qu < ARRSZ(queues_seen); qu++) {
            if (!queues_seen[qu]) {
                T_EG(TRACE_GRP_NETLINK, "Didn't get data for qu #%u", qu);
                ok = false;
                // Keep going.
            }
        }
    }

    bool ok = false;
    packet_throttle_cfg_t throttle_cfg;
};

/******************************************************************************/
// RX_throttle_cfg_netlink_get()
// This function makes a netlink request from the kernel and decomposes it into
// a packet_throttle_cfg_t.
/******************************************************************************/
static mesa_rc RX_throttle_cfg_netlink_get(packet_throttle_cfg_t *packet_throttle_cfg)
{
    if (!CX_internal_mips) {
        // Only supported on internal MIPS CPU
        return VTSS_RC_ERROR;
    }

    CX_netlink_rx_throttle_cfg_get capture;
    CX_netlink_req                 req(VTSS_PACKET_GENL_RX_THROTTLE_CFG_GET);

    if (!packet_throttle_cfg) {
        return VTSS_RC_ERROR;
    }

    INVOKE_FUNC(vtss::appl::netlink::genl_req, (const void *)&req, req.netlink_msg_hdr.nlmsg_len, req.netlink_msg_hdr.nlmsg_seq, __FUNCTION__, &capture);

    if (!capture.ok) {
        T_EG(TRACE_GRP_NETLINK, "Netlink get of throttle config failed");
        return VTSS_RC_ERROR;
    }

    *packet_throttle_cfg = capture.throttle_cfg;
    return VTSS_RC_OK;
}

/****************************************************************************/
// RX_throttle_cfg_netlink_set()
// This function composes a netlink request and sends it to the kernel,
// which will take care of using the throttling parameters.
/****************************************************************************/
static mesa_rc RX_throttle_cfg_netlink_set(const packet_throttle_cfg_t *packet_throttle_cfg)
{
    if (!CX_internal_mips) {
        // Only supported on internal MIPS CPU
        return VTSS_RC_ERROR;
    }

    CX_netlink_req         req(VTSS_PACKET_GENL_RX_THROTTLE_CFG_SET);
    mesa_packet_rx_queue_t qu;

    if (!packet_throttle_cfg) {
        return VTSS_RC_ERROR;
    }

    CX_NETLINK_ATTR_ADD(VTSS_PACKET_ATTR_RX_THROTTLE_TICK_PERIOD_MSEC, u32, PACKET_THROTTLE_PERIOD_MS);

    for (qu = 0; qu < ARRSZ(packet_throttle_cfg->frm_limit_per_tick); qu++) {
        struct rtattr *element;

        if ((element = vtss::appl::netlink::attr_nest(&req.netlink_msg_hdr, req.max_size_bytes, VTSS_PACKET_ATTR_RX_THROTTLE_QU_CFG | NLA_F_NESTED)) == NULL) {
            T_EG(TRACE_GRP_NETLINK, "netlink::attr_nest() failed");
            return VTSS_RC_ERROR;
        }

        CX_NETLINK_ATTR_ADD(VTSS_PACKET_ATTR_RX_THROTTLE_QU_NUMBER,           u32, qu);
        CX_NETLINK_ATTR_ADD(VTSS_PACKET_ATTR_RX_THROTTLE_FRM_LIMIT_PER_TICK,  u32, packet_throttle_cfg->frm_limit_per_tick[qu]);
        CX_NETLINK_ATTR_ADD(VTSS_PACKET_ATTR_RX_THROTTLE_BYTE_LIMIT_PER_TICK, u32, packet_throttle_cfg->byte_limit_per_tick[qu]);
        CX_NETLINK_ATTR_ADD(VTSS_PACKET_ATTR_RX_THROTTLE_SUSPEND_TICK_CNT,    u32, packet_throttle_cfg->suspend_tick_cnt[qu]);

        vtss::appl::netlink::attr_nest_end(&req.netlink_msg_hdr, element);
    }

    T_DG(TRACE_GRP_NETLINK, "Sending the following %u bytes @ %p to the kernel", req.netlink_msg_hdr.nlmsg_len, &req.netlink_msg_hdr);
    T_DG_HEX(TRACE_GRP_NETLINK, (const uchar *)&req.netlink_msg_hdr, req.netlink_msg_hdr.nlmsg_len);

    INVOKE_FUNC(vtss::appl::netlink::genl_req, (const void *)&req, req.netlink_msg_hdr.nlmsg_len, req.netlink_msg_hdr.nlmsg_seq, __FUNCTION__);

    return VTSS_RC_OK;
}

/****************************************************************************/
// RX_throttle_cfg_mesa_get()
/****************************************************************************/
static mesa_rc RX_throttle_cfg_mesa_get(packet_throttle_cfg_t *packet_throttle_cfg)
{
    mesa_packet_rx_conf_t rx_conf;
    mesa_rc               rc;
    int                   qu;

    memset(packet_throttle_cfg, 0, sizeof(*packet_throttle_cfg));
    PACKET_CHECK((rc = mesa_packet_rx_conf_get(nullptr, &rx_conf)) == VTSS_RC_OK, return rc;);

    for (qu = 0; qu < ARRSZ(packet_throttle_cfg->frm_limit_per_tick); qu++) {
        packet_throttle_cfg->frm_limit_per_tick[qu] = rx_conf.queue[qu].rate == MESA_PACKET_RATE_DISABLED ? 0 : rx_conf.queue[qu].rate / PACKET_THROTTLE_FREQ_HZ;
    }

    return VTSS_RC_OK;
}

/****************************************************************************/
// RX_throttle_cfg_mesa_set()
/****************************************************************************/
static mesa_rc RX_throttle_cfg_mesa_set(const packet_throttle_cfg_t *packet_throttle_cfg)
{
    mesa_packet_rx_conf_t rx_conf;
    mesa_rc               rc;
    int                   qu;

    PACKET_CHECK((rc = mesa_packet_rx_conf_get(nullptr, &rx_conf)) == VTSS_RC_OK, return rc;);

    for (qu = 0; qu < ARRSZ(rx_conf.queue); qu++) {
        rx_conf.queue[qu].rate = packet_throttle_cfg->frm_limit_per_tick[qu] == 0 ? MESA_PACKET_RATE_DISABLED : packet_throttle_cfg->frm_limit_per_tick[qu] * PACKET_THROTTLE_FREQ_HZ;
    }

    PACKET_CHECK((rc = mesa_packet_rx_conf_set(nullptr, &rx_conf)) == VTSS_RC_OK, return rc;);

    return VTSS_RC_OK;
}

/******************************************************************************/
// CX_netlink_rx_cfg_get class
// Contains a callback function invoked when current Rx config is retrieved from
// the kernel.
/******************************************************************************/
class CX_netlink_rx_cfg_get : public vtss::appl::netlink::NetlinkCallbackAbstract
{
public:
    void operator()(struct sockaddr_nl *addr, struct nlmsghdr *netlink_msg_hdr)
    {
        struct genlmsghdr *genl;
        int               len;
        struct rtattr     *attr;
        bool              mtu_seen = false;

        T_DG(TRACE_GRP_NETLINK, "I got in here");

        genl = (struct genlmsghdr *)NLMSG_DATA(netlink_msg_hdr);
        if ((len  = netlink_msg_hdr->nlmsg_len - NLMSG_LENGTH(sizeof(*genl))) < 0) {
            T_EG(TRACE_GRP_NETLINK, "Msg too short for this type (%d)!", len);
            return;
        }

        memset(&rx_cfg, 0, sizeof(rx_cfg));

        attr = GENL_RTA(genl);

        T_DG(TRACE_GRP_NETLINK, "Whole payload (starts @ %p)", attr);
        T_DG_HEX(TRACE_GRP_NETLINK, (const uchar *)attr, len);

        // An attribute consists of:
        // attr->rta_len:  a 2 byte length, which includes itself, attr->rta_type, and the data following it.
        // attr->rta_type: a 2 byte attribute index (e.g. VTSS_PACKET_ATTR_RX_CFG_MTU).
        // Payload:        RTA_PAYLOAD(attr) bytes of data forming the value of the attribute.

        while (RTA_OK(attr, len)) {
            T_DG(TRACE_GRP_NETLINK, "RTA_PAYLOAD(attr) = %u, attr->rta_len = %u. Dumping attr->rta_len bytes from %p, which is attr ID = 0x%x", RTA_PAYLOAD(attr), attr->rta_len, attr, attr->rta_type);
            T_DG_HEX(TRACE_GRP_NETLINK, (const uchar *)attr, attr->rta_len);

            switch (attr->rta_type & NLA_TYPE_MASK) {
            case VTSS_PACKET_ATTR_RX_CFG_MTU:
                CX_NETLINK_ATTR_U32_GET(attr, rx_cfg.mtu);
                mtu_seen = true;
                break;

            default:
                T_EG(TRACE_GRP_NETLINK, "Unknown attribute %hu", attr->rta_type);
                return;
            }

            attr = RTA_NEXT(attr, len);
        }

        ok = true;

        // Final checks.

        // See if we got a VTSS_PACKET_ATTR_RX_CFG_MTU attribute
        if (!mtu_seen) {
            T_EG(TRACE_GRP_NETLINK, "Didn't get an MTU attribute");
            ok = false;
            // Keep going.
        }
    }

    bool ok = false;
    packet_rx_cfg_t rx_cfg;
};

/******************************************************************************/
// packet_rx_cfg_get()
// This function makes a netlink request from the kernel and decomposes it into
// a packet_rx_cfg_t.
/******************************************************************************/
mesa_rc packet_rx_cfg_get(packet_rx_cfg_t *rx_cfg)
{
    if (!CX_internal_mips) {
        // Only supported on internal MIPS CPU
        return VTSS_RC_ERROR;
    }

    CX_netlink_rx_cfg_get capture;
    CX_netlink_req        req(VTSS_PACKET_GENL_RX_CFG_GET);

    if (!rx_cfg) {
        return VTSS_RC_ERROR;
    }

    INVOKE_FUNC(vtss::appl::netlink::genl_req, (const void *)&req, req.netlink_msg_hdr.nlmsg_len, req.netlink_msg_hdr.nlmsg_seq, __FUNCTION__, &capture);

    if (!capture.ok) {
        T_EG(TRACE_GRP_NETLINK, "Netlink get of rx config failed");
        return VTSS_RC_ERROR;
    }

    *rx_cfg = capture.rx_cfg;
    return VTSS_RC_OK;
}

/****************************************************************************/
// packet_rx_cfg_set()
// This function composes a netlink request and sends it to the kernel,
// which will take care of using the parameters.
/****************************************************************************/
mesa_rc packet_rx_cfg_set(packet_rx_cfg_t *rx_cfg)
{
    if (!CX_internal_mips) {
        // Only supported on internal MIPS CPU
        return VTSS_RC_ERROR;
    }

    CX_netlink_req req(VTSS_PACKET_GENL_RX_CFG_SET);

    if (!rx_cfg) {
        return VTSS_RC_ERROR;
    }

    CX_NETLINK_ATTR_ADD(VTSS_PACKET_ATTR_RX_CFG_MTU, u32, rx_cfg->mtu);

    T_DG(TRACE_GRP_NETLINK, "Sending the following %u bytes @ %p to the kernel", req.netlink_msg_hdr.nlmsg_len, &req.netlink_msg_hdr);
    T_DG_HEX(TRACE_GRP_NETLINK, (const uchar *)&req.netlink_msg_hdr, req.netlink_msg_hdr.nlmsg_len);

    INVOKE_FUNC(vtss::appl::netlink::genl_req, (const void *)&req, req.netlink_msg_hdr.nlmsg_len, req.netlink_msg_hdr.nlmsg_seq, __FUNCTION__);

    // That will be our new MTU.
    RX_mtu = rx_cfg->mtu;

    return VTSS_RC_OK;
}

/****************************************************************************/
// packet_rx_process_set()
/****************************************************************************/
void packet_rx_process_set(bool val)
{
    RX_processing = val;
}

/****************************************************************************/
// CX_netlink_trace_cfg_set()
// This function composes a netlink request and sends it to the kernel,
// which will take care of using the trace parameters.
/****************************************************************************/
static mesa_rc CX_netlink_trace_cfg_set(u32 layer, u32 group, u32 level)
{
    if (!CX_internal_mips) {
        // Only supported on internal MIPS CPU
        return VTSS_RC_ERROR;
    }

    CX_netlink_req req(VTSS_PACKET_GENL_TRACE_CFG_SET);

    CX_NETLINK_ATTR_ADD(VTSS_PACKET_ATTR_TRACE_LAYER, u32, layer);
    CX_NETLINK_ATTR_ADD(VTSS_PACKET_ATTR_TRACE_GROUP, u32, group);
    CX_NETLINK_ATTR_ADD(VTSS_PACKET_ATTR_TRACE_LEVEL, u32, level);

    T_DG(TRACE_GRP_NETLINK, "Sending the following %u bytes @ %p to the kernel", req.netlink_msg_hdr.nlmsg_len, &req.netlink_msg_hdr);
    T_DG_HEX(TRACE_GRP_NETLINK, (const uchar *)&req.netlink_msg_hdr, req.netlink_msg_hdr.nlmsg_len);

    INVOKE_FUNC(vtss::appl::netlink::genl_req, (const void *)&req, req.netlink_msg_hdr.nlmsg_len, req.netlink_msg_hdr.nlmsg_seq, __FUNCTION__);

    return VTSS_RC_OK;
}

/****************************************************************************/
// CX_ufdma_stati_clear()
// This function composes a netlink request and sends it to the kernel,
// which will take care of clearing ufdma statistics.
/****************************************************************************/
static void CX_ufdma_stati_clear(void)
{
    if (!CX_internal_mips) {
        // Only supported on internal MIPS CPU
        return;
    }

    CX_netlink_req req(VTSS_PACKET_GENL_STATI_CLEAR);

    T_DG(TRACE_GRP_NETLINK, "Sending the following %u bytes @ %p to the kernel", req.netlink_msg_hdr.nlmsg_len, &req.netlink_msg_hdr);
    T_DG_HEX(TRACE_GRP_NETLINK, (const uchar *)&req.netlink_msg_hdr, req.netlink_msg_hdr.nlmsg_len);

    (void)vtss::appl::netlink::genl_req((const void *)&req, req.netlink_msg_hdr.nlmsg_len, req.netlink_msg_hdr.nlmsg_seq, __FUNCTION__);
}

/****************************************************************************/
// RX_throttle_init()
/****************************************************************************/
static void RX_throttle_init(void)
{
    packet_throttle_cfg_t  throttle_cfg;
    mesa_packet_rx_queue_t qu;
    mesa_rc                rc;

    memset(&throttle_cfg, 0, sizeof(throttle_cfg));

    // This will set the sFlow extraction queue to a maximum of 300 frames per second.
    // A suspend tick count of 0 means that it will extract up to 300 / FREQ_HZ frames per tick, then suspend and
    // re-open upon the next tick.
    throttle_cfg.frm_limit_per_tick[PACKET_XTR_QU_SFLOW]      = 300  /* frames per second */ / PACKET_THROTTLE_FREQ_HZ;
    throttle_cfg.suspend_tick_cnt[PACKET_XTR_QU_SFLOW]        = 0    /* milliseconds      */ / PACKET_THROTTLE_PERIOD_MS;

    // This will set the broadcast queue to a maximum of 500 frames per second.
    throttle_cfg.frm_limit_per_tick[PACKET_XTR_QU_BC]         = 500  /* frames per second */ / PACKET_THROTTLE_FREQ_HZ;
    throttle_cfg.suspend_tick_cnt[PACKET_XTR_QU_BC]           = 0    /* milliseconds      */ / PACKET_THROTTLE_PERIOD_MS;

#if PACKET_XTR_QU_ACL_REDIR != PACKET_XTR_QU_ACL_COPY
    throttle_cfg.frm_limit_per_tick[PACKET_XTR_QU_ACL_REDIR]  = 300  /* frames per second */ / PACKET_THROTTLE_FREQ_HZ;
    throttle_cfg.suspend_tick_cnt[PACKET_XTR_QU_ACL_REDIR]    = 0    /* milliseconds      */ / PACKET_THROTTLE_PERIOD_MS;
#endif /* PACKET_XTR_QU_ACL_REDIR != PACKET_XTR_QU_ACL_COPY */

    // This will set the management queue to a maximum of 3000 frames per second.
    throttle_cfg.frm_limit_per_tick[PACKET_XTR_QU_MGMT_MAC]   = 3000 /* frames per second */ / PACKET_THROTTLE_FREQ_HZ;
    throttle_cfg.suspend_tick_cnt[PACKET_XTR_QU_MGMT_MAC]     = 0    /* milliseconds      */ / PACKET_THROTTLE_PERIOD_MS;

    throttle_cfg.frm_limit_per_tick[PACKET_XTR_QU_L3_OTHER]   = 500  /* frames per second */ / PACKET_THROTTLE_FREQ_HZ;
    throttle_cfg.suspend_tick_cnt[PACKET_XTR_QU_L3_OTHER]     = 0    /* milliseconds      */ / PACKET_THROTTLE_PERIOD_MS;

#if defined(VTSS_SW_OPTION_MEP)
    throttle_cfg.frm_limit_per_tick[PACKET_XTR_QU_OAM]        = 1300 /* frames per second */ / PACKET_THROTTLE_FREQ_HZ;
    throttle_cfg.suspend_tick_cnt[PACKET_XTR_QU_OAM]          = 0    /* milliseconds      */ / PACKET_THROTTLE_PERIOD_MS;
#endif /* VTSS_SW_OPTION_MEP */

#if defined(VTSS_SW_OPTION_IPMC)
    // This will set IPMC queue to a maximum of 1500 frames per second.
    throttle_cfg.frm_limit_per_tick[PACKET_XTR_QU_IGMP]    = 1500  /* frames per second */ / PACKET_THROTTLE_FREQ_HZ;
    throttle_cfg.suspend_tick_cnt[PACKET_XTR_QU_IGMP]      = 0    /* milliseconds      */ / PACKET_THROTTLE_PERIOD_MS;
#endif

    // This will set the BPDU queue to a maximum of 1000 frames per second.
    throttle_cfg.frm_limit_per_tick[PACKET_XTR_QU_BPDU]       = 1000 /* frames per second */ / PACKET_THROTTLE_FREQ_HZ;
    throttle_cfg.suspend_tick_cnt[PACKET_XTR_QU_BPDU]         = 0    /* milliseconds      */ / PACKET_THROTTLE_PERIOD_MS;

    throttle_cfg.frm_limit_per_tick[PACKET_XTR_QU_LRN_ALL]    = 500  /* frames per second */ / PACKET_THROTTLE_FREQ_HZ;
    throttle_cfg.suspend_tick_cnt[PACKET_XTR_QU_LRN_ALL]      = 0    /* milliseconds      */ / PACKET_THROTTLE_PERIOD_MS;

    // Gotta set the byte-limits too. In lack of any better, we set it to frame limit times MTU.
    // In this way, we will shut down based on byte limit if jumbo frames are received.
    for (qu = 0; qu < ARRSZ(throttle_cfg.byte_limit_per_tick); qu++) {
        throttle_cfg.byte_limit_per_tick[qu] = throttle_cfg.frm_limit_per_tick[qu] * 1518;
    }

    if ((rc = packet_rx_throttle_cfg_set(&throttle_cfg)) != VTSS_RC_OK && rc != PACKET_RC_THROTTLING_NOT_SUPPORTED) {
        T_E("packet_rx_throttle_cfg_set() failed: %s", error_txt(rc));
    }
}

/****************************************************************************/
// RX_shaping_init()
/****************************************************************************/
static void RX_shaping_init(void)
{
    u64 rate_kbps;

    // If CPU port shaping is supported, we can enable it to limit the total
    // number of frames (bits) arriving at the CPU. In combination with
    // throttling, this indeed makes sense, because the throttle mechanism
    // is a per-Rx-queue property, whereas the shaping mechanism is a per-
    // port (all 8 Rx queues) property. It could be that the throttling
    // allows a particular queue to let so and so many frames through,
    // but that the combination of all queues will exceed the CPU's ability
    // to handle the frames. This is where port-shaping comes in. It will
    // limit the total amount (port-shaping could not be implemented in software
    // lke the per-queue throttling, because that would shut down all 8 queues
    // if port-throttling kicks in, which is not desirable).
    // So what should the shaper rate be set to? It must at least be set to
    // the maximum per-queue rate set with throttling. Here, I have chosen to
    // set it to the sum of the allowed BPDU and Management rate.
    rate_kbps = (3000LLU + 1000LLU); // Frames
    rate_kbps *= (1518LLU + 20LLU);  // Max bytes per frame + IFG + Preamble, since rate is in line (L1), not data (L2).
    rate_kbps *= 8LLU;               // Bits per byte
    rate_kbps /= 1000LL;             // From bps to kbps.
    (void)packet_rx_shaping_cfg_set(rate_kbps);
}

/******************************************************************************/
// TX_npi_do()
/******************************************************************************/
static mesa_rc TX_npi_do(const mesa_packet_tx_info_t *tx_info, u8 *frm_ptr, u32 frm_len)
{
    u8            ifh[MESA_PACKET_HDR_SIZE_BYTES];
    u32           ifh_len = sizeof(ifh);
    struct msghdr hdr;
    struct iovec  iov[3];
    mesa_rc       rc = VTSS_RC_ERROR;

    if (TX_trace_to_file) {
        CX_trace_to_file(frm_ptr, frm_len);
    }

    if (CX_internal_cpu) {
        if ((rc = mesa_packet_tx_hdr_encode(NULL, tx_info, ifh_len, (u8 *)ifh, &ifh_len)) != VTSS_RC_OK) {
            T_EG(TRACE_GRP_TX, "mesa_packet_tx_hdr_encode() failed");
            return rc;
        }

        T_DG(TRACE_GRP_TX, "%u bytes IFH", ifh_len);
        T_DG_HEX(TRACE_GRP_TX, ifh, ifh_len);

        T_NG(TRACE_GRP_TX, "%u bytes frame (w/o IFH and w/o FCS)", frm_len);
        T_NG_HEX(TRACE_GRP_TX, frm_ptr, MIN(96, frm_len));

        // Format the message and send it on the IFH interface
        iov[0].iov_base = npi_encap;
        iov[0].iov_len  = sizeof(npi_encap);
        iov[1].iov_base = ifh;
        iov[1].iov_len  = ifh_len;
        iov[2].iov_base = frm_ptr;
        iov[2].iov_len  = frm_len;

        memset(&hdr, 0, sizeof(hdr));
        hdr.msg_name    = &npi_socket_address;
        hdr.msg_namelen = sizeof(npi_socket_address);
        hdr.msg_iov     = iov;
        hdr.msg_iovlen  = ARRSZ(iov);

        if (sendmsg(ifh_sock, &hdr, 0) > 0) {
            T_DG(TRACE_GRP_TX, "IFH xmit: sent OK");
            rc = VTSS_RC_OK;
        } else {
            T_E("IFH xmit: Error = %s", strerror(errno));
            T_EG_HEX(TRACE_GRP_TX, ifh, ifh_len);
            T_EG_HEX(TRACE_GRP_TX, frm_ptr, frm_len);
        }
    } else {
        T_NG(TRACE_GRP_TX, "%u bytes frame (w/o IFH and w/o FCS)", frm_len);
        T_NG_HEX(TRACE_GRP_TX, frm_ptr, MIN(96, frm_len));

        if ((rc = mesa_packet_tx_frame(nullptr, tx_info, frm_ptr, frm_len)) != VTSS_RC_OK) {
            T_E("mesa_packet_tx_frame() failed: %s", error_txt(rc));
            T_EG_HEX(TRACE_GRP_TX, ifh, ifh_len);
            T_EG_HEX(TRACE_GRP_TX, frm_ptr, frm_len);
        }
    }

    return rc;
}

/******************************************************************************/
// TX_props_modify_for_redbox()
/******************************************************************************/
static const packet_tx_props_t *TX_props_modify_for_redbox(const uint8_t *frm_ptr, const packet_tx_props_t *tx_props, packet_tx_props_t *alternative_tx_props)
{
#ifdef VTSS_SW_OPTION_REDBOX
    static const uint8_t bpdu_mac[] = {0x01, 0x80, 0xc2, 0x00, 0x00};

    // On systems with RedBox support, we need to modify a tx_info flag if we
    // transmit a BPDU. This is in order to prevent the BPDU from being HSR-
    // tagged on its way out of an LRE port (the RedBox' Port A/B). The bit can
    // always be set, even when transmitting to non-LRE ports.
    // Only MAC addresses from 802.1Q-2018, Table 8-1, 8-2, and 8-3 are
    // relevant. These are addresses in the following range:
    //   01-80-c2-00-00-00 through 01-80-c2-00-00-0f
    // Notice: The rb_tag_disable does not affect the RCT in PRP-SAN mode.
    if (memcmp(frm_ptr, bpdu_mac, sizeof(bpdu_mac)) == 0 && frm_ptr[5] <= 0x0f) {
        // Gotta take a copy of the tx_props and modify and return the copy,
        // because the tx_props are declared const and can't be modified.
        *alternative_tx_props = *tx_props;

        // Ask it *not* to HSR-tag the frame
        alternative_tx_props->tx_info.rb_tag_disable = true;

        // And if we Tx too fast, ask it not to discard any of them.
        alternative_tx_props->tx_info.rb_dd_disable  = true;
        return alternative_tx_props;
    }
#endif

    return tx_props;
}

/******************************************************************************/
// TX_npi()
// Store tx_props in Tx pending fifo.
/******************************************************************************/
static mesa_rc TX_npi(const packet_tx_props_t *tx_props)
{
    mesa_rc                     rc = VTSS_RC_OK;
    u8                          *frm_ptr;
    u32                         frm_len;
    u64                         dst_port;
    mesa_packet_tx_info_t       tx_info_local;
    BOOL                        insert_tpid_tag, insert_masq_tag, insert_clas_tag;
    const mesa_packet_tx_info_t *tx_info;
    packet_tx_props_t           alternative_tx_props;
    const packet_tx_props_t     *resulting_tx_props;

    if (ifh_sock < 0) {
        T_EG(TRACE_GRP_TX, "IFH Tx: No IFH interface open");
        return VTSS_RC_ERROR;
    }

    frm_ptr = tx_props->packet_info.frm;
    frm_len = tx_props->packet_info.len;

    if (!CX_internal_cpu) {
        // If using an external CPU, the frame is transmitted to the switch's NPI
        // port with an encapsulation header. The switch strips this header and if
        // the resulting underlying frame is smaller than a minimum-sized Ethernet
        // frame, the switch may send it out as an undersized frame. Therefore, we
        // must ensure here, that the frame is long enough. If using the internal
        // CPU, the FDMA driver does this check prior to Tx.
        // frm_len is the size of the frame without FCS.
        frm_len = MAX(frm_len, 60);
    }

    // Possibly insert a VLAN tag.
    tx_info = &tx_props->tx_info;
    insert_tpid_tag = (!tx_info->switch_frm && tx_info->tag.tpid != 0);
    if (TX_insert_tag) {
        insert_masq_tag = (tx_info->switch_frm && tx_info->masquerade_port != VTSS_PORT_NO_NONE && tx_info->tag.vid != 0);
    } else {
        // Never insert a tag due to masquerading on e.g. JR2, because the user of this feature
        // uses pipeline injection points rather than real port looping.
        insert_masq_tag = FALSE;
    }

    insert_clas_tag = (tx_info->switch_frm && tx_info->masquerade_port == VTSS_PORT_NO_NONE);

    if (insert_tpid_tag || insert_masq_tag || insert_clas_tag) {
        mesa_vlan_tag_t      the_tag;
        const mesa_vlan_tag_t *tag;
        u16                   tci;

        if (insert_masq_tag || insert_clas_tag) {
            // We gotta insert a VLAN tag into the frame when switching the frame.
            // The alternative to this is to use the VStaX header to hold the tag
            // and set FWD.VSTAX_AVAIL, but this will not work in a stacking environment
            // (switched frames will not go across the stack ports then, and TTL cannot
            // be set-up, since we don't know which way the frame goes).
            the_tag.tpid = 0x8100;
            the_tag.pcp  = tx_info->cos >= 8 ? 7 : tx_info->cos; // Even though there is a PCP-to-CoS conversion.
            the_tag.dei  = 0;
            the_tag.vid  = tx_info->tag.vid;
        }

        // Use tag from tx_info when asked to insert a tag (tpid != 0).
        tag = insert_masq_tag || insert_clas_tag ? &the_tag : &tx_info->tag;

        // We know that the buffer was allocated with packet_tx_alloc() or
        // packet_tx_alloc_extra(), which means that we know that there is room
        // for an additional tag ahead of tx_props->packet_info.frm.
        frm_ptr -= 4;
        frm_len += 4;

        // Move DMAC + SMAC four bytes back
        memmove(frm_ptr, frm_ptr + 4, ETYPE_POS);

        // Insert the tag
        CX_htons(&frm_ptr[ETYPE_POS], tag->tpid);
        tci = (tag->pcp << 13) | (tag->dei << 12) | tag->vid;
        CX_htons(&frm_ptr[ETYPE_POS + 2], tci);

        T_NG(TRACE_GRP_TX, "Inserted VLAN tag with TPID = 0x%04x and VID = %d", tag->tpid, tag->vid);
    }

    resulting_tx_props = TX_props_modify_for_redbox(frm_ptr, tx_props, &alternative_tx_props);

    // Not all chips support the mesa_packet_tx_info_t::dst_port_mask field.
    packet_debug_tx_props_print(VTSS_TRACE_MODULE_ID, TRACE_GRP_TX, VTSS_TRACE_LVL_NOISE, resulting_tx_props);

    if (TX_ifh_has_dst_port_mask || tx_info->dst_port_mask == 0) {
        // This chip supports the dst_port_mask field or the destination port
        // mask is empty (frame is switched). Nothing more to do.
        rc = TX_npi_do(tx_info, frm_ptr, frm_len);
    } else {
        // This chip doesn't support the dst_port_mask field. Go through all
        // bits and send the frame one by one.
        tx_info_local = *tx_info;
        tx_info_local.dst_port_mask = 0;
        for (dst_port = 0; dst_port < CX_port_cnt; dst_port++) {
            if (!(tx_info->dst_port_mask & VTSS_BIT64(dst_port))) {
                continue;
            }

            tx_info_local.dst_port = dst_port;
            if ((rc = TX_npi_do(&tx_info_local, frm_ptr, frm_len)) != VTSS_RC_OK) {
                break;
            }
        }
    }

    if (!resulting_tx_props->packet_info.no_free) {
        packet_tx_free(resulting_tx_props->packet_info.frm);
    }

    return rc;
}

/****************************************************************************
 * TX_mask_analyze()
 * Get the number of bits set in #mask.
 * Returns 0 if no bits are set, 1 if exactly one bit is set, 2 if at least
 * two bits are set.
 * If exactly one bit is set, bit_pos holds the bit position.
 ****************************************************************************/
static u32 TX_mask_analyze(u64 mask, u32 *bit_pos)
{
    u32 i, w, p, cnt = 0;

    if (mask == 0) {
        return 0;
    }

    for (i = 0; i < 2; i ++) {
        w = (u32)(mask >> (32 * i));

        if ((p = VTSS_OS_CTZ(w)) < 32) {
            w &= ~VTSS_BIT(p);
            if (w) {
                // Still bits set in w.
                return 2;
            }
            cnt++;
            *bit_pos = p + 32 * i;
        }
    }

    return cnt > 1 ? 2 : cnt;
}

/****************************************************************************
 * TX_oam_type_to_str()
 ****************************************************************************/
static const char *TX_oam_type_to_str(mesa_packet_oam_type_t oam_type)
{
    switch (oam_type) {
    case MESA_PACKET_OAM_TYPE_NONE:
        return "None";

    case MESA_PACKET_OAM_TYPE_CCM:
        return "CCM";

    case MESA_PACKET_OAM_TYPE_CCM_LM:
        return "CCM_LM";

    case MESA_PACKET_OAM_TYPE_LBM:
        return "LBM";

    case MESA_PACKET_OAM_TYPE_LBR:
        return "LBR";

    case MESA_PACKET_OAM_TYPE_LMM:
        return "LMM";

    case MESA_PACKET_OAM_TYPE_LMR:
        return "LMR";

    case MESA_PACKET_OAM_TYPE_DMM:
        return "DMM";

    case MESA_PACKET_OAM_TYPE_DMR:
        return "DMR";

    case MESA_PACKET_OAM_TYPE_1DM:
        return "1DM";

    case MESA_PACKET_OAM_TYPE_LTM:
        return "LTM";

    case MESA_PACKET_OAM_TYPE_LTR:
        return "LTR";

    case MESA_PACKET_OAM_TYPE_GENERIC:
        return "Generic";

    case MESA_PACKET_OAM_TYPE_LCK:
        return "LCK";

    case MESA_PACKET_OAM_TYPE_MPLS_TP_1:
        return "MPLS_TP_1";

    case MESA_PACKET_OAM_TYPE_MPLS_TP_2:
        return "MPLS_TP_2";

    case MESA_PACKET_OAM_TYPE_MRP_TST:
        return "MRP_Test";

    case MESA_PACKET_OAM_TYPE_MRP_ITST:
        return "MRP_InTest";

    case MESA_PACKET_OAM_TYPE_DLR_BCN:
        return "DLR_Beacon";

    case MESA_PACKET_OAM_TYPE_DLR_ADV:
        return "DLR_Advertise";

    default:
        break;
    }

    return "Unknown";
}

/****************************************************************************
 * TX_pipeline_pt_to_str()
 ****************************************************************************/
static const char *TX_pipeline_pt_to_str(mesa_packet_pipeline_pt_t pipeline_pt)
{
    switch (pipeline_pt) {
    case MESA_PACKET_PIPELINE_PT_NONE:
        return "None";

    case MESA_PACKET_PIPELINE_PT_ANA_PORT_VOE:
        return "ANA_PORT_VOE";

    case MESA_PACKET_PIPELINE_PT_ANA_CL:
        return "ANA_CL";

    case MESA_PACKET_PIPELINE_PT_ANA_CLM:
        return "ANA_CLM";

    case MESA_PACKET_PIPELINE_PT_ANA_OU_VOI:
        return "ANA_OU_VOI";

    case MESA_PACKET_PIPELINE_PT_ANA_OU_SW:
        return "ANA_OU_SW";

    case MESA_PACKET_PIPELINE_PT_ANA_OU_VOE:
        return "ANA_OU_VOE";

    case MESA_PACKET_PIPELINE_PT_ANA_IN_VOE:
        return "ANA_IN_VOE";

    case MESA_PACKET_PIPELINE_PT_ANA_IN_SW:
        return "ANA_IN_SW";

    case MESA_PACKET_PIPELINE_PT_ANA_IN_VOI:
        return "ANA_IN_VOI";

    case MESA_PACKET_PIPELINE_PT_REW_IN_VOI:
        return "REW_IN_VOI";

    case MESA_PACKET_PIPELINE_PT_REW_IN_SW:
        return "REW_IN_SW";

    case MESA_PACKET_PIPELINE_PT_REW_IN_VOE:
        return "REW_IN_VOE";

    case MESA_PACKET_PIPELINE_PT_REW_OU_VOE:
        return "REW_OU_VOE";

    case MESA_PACKET_PIPELINE_PT_REW_OU_SW:
        return "REW_OU_SW";

    case MESA_PACKET_PIPELINE_PT_REW_OU_VOI:
        return "REW_OU_VOI";

    case MESA_PACKET_PIPELINE_PT_REW_PORT_VOE:
        return "REW_PORT_VOE";

    default:
        break;
    }

    return "Unknown";
}

/****************************************************************************
 * TX_ptp_action_to_str()
 ****************************************************************************/
static const char *TX_ptp_action_to_str(mesa_packet_ptp_action_t ptp_action)
{
    switch (ptp_action) {
    case MESA_PACKET_PTP_ACTION_NONE:
        return "None";

    case MESA_PACKET_PTP_ACTION_ONE_STEP:
        return "ONE_STEP";

    case MESA_PACKET_PTP_ACTION_TWO_STEP:
        return "TWO_STEP";

    case MESA_PACKET_PTP_ACTION_ONE_AND_TWO_STEP:
        return "ONE_AND_TWO_STEP";

    case MESA_PACKET_PTP_ACTION_ORIGIN_TIMESTAMP:
        return "ORIGIN_TIMESTAMP";

    case MESA_PACKET_PTP_ACTION_ORIGIN_TIMESTAMP_SEQ:
        return "ORIGIN_TIMESTAMP_SEQ";

    case MESA_PACKET_PTP_ACTION_AFI_NONE:
        return "AFI_NONE";

    default:
        break;
    }

    return "Unknown";
}

/******************************************************************************/
// TX_rb_fwd_to_str()
/******************************************************************************/
static const char *TX_rb_fwd_to_str(mesa_packet_rb_fwd_t rb_fwd)
{
    switch (rb_fwd) {
    case MESA_PACKET_RB_FWD_DEFAULT:
        return "Default";

    case MESA_PACKET_RB_FWD_A:
        return "Port A";

    case MESA_PACKET_RB_FWD_B:
        return "Port B";

    case MESA_PACKET_RB_FWD_BOTH:
        return "Both";

    default:
        break;
    }

    return "Unknown";
}

/******************************************************************************/
// packet_error_txt()
/******************************************************************************/
const char *packet_error_txt(mesa_rc rc)
{
    switch (rc) {
    case PACKET_RC_GEN:
        return "General error";

    case PACKET_RC_PARAM:
        return "Invalid parameter";

    case PACKET_RC_TX_CHECK:
        return "packet_tx(): Invalid parameter found in tx_props";

    case PACKET_RC_FDMA_TX:
        return "vtss_fdma_tx() failed";

    case PACKET_RC_FDMA_AFI_CANCEL_FRAME_NOT_FOUND:
        return "AFI frame was not found";

    case PACKET_RC_THROTTLING_NOT_SUPPORTED:
        return "Rx throttling is not supported on this platform";

    default:
        return "Unknown packet module error";
    }
}

/****************************************************************************/
// CX_ufdma_appl2api_trace_lvl()
// Converts application trace level to uFDMA trace level
/****************************************************************************/
static u32 CX_ufdma_appl2api_trace_lvl(int global_lvl, int lvl)
{
    if (global_lvl > lvl) {
        lvl = global_lvl;
    }

    switch (lvl) {
    case VTSS_TRACE_LVL_ERROR:
    case VTSS_TRACE_LVL_WARNING:
        return UFDMA_TRACE_LEVEL_ERROR;

    case VTSS_TRACE_LVL_INFO:
        return UFDMA_TRACE_LEVEL_INFO;

    case VTSS_TRACE_LVL_DEBUG:
    case VTSS_TRACE_LVL_NOISE:
    case VTSS_TRACE_LVL_RACKET:
        return UFDMA_TRACE_LEVEL_DEBUG;

    case VTSS_TRACE_LVL_NONE:
        return UFDMA_TRACE_LEVEL_NONE;

    default:
        return UFDMA_TRACE_LEVEL_ERROR; /* Should never happen */
    }
}

/****************************************************************************/
/*                                                                          */
/*  MODULE EXTERNAL FUNCTIONS                                               */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
// packet_ufdma_trace_update()
/****************************************************************************/
void packet_ufdma_trace_update(void)
{
    int global_lvl;
    u32 grp;

    if (!CX_internal_mips) {
        // Only supported on internal MIPS CPU
        return;
    }

    global_lvl = vtss_trace_global_lvl_get();

    for (grp = 0; grp < ARRSZ(ufdma_ail_trace_grps); grp++) {
        // Map WebStaX trace level to uFDMA API trace level
        CX_netlink_trace_cfg_set(UFDMA_TRACE_LAYER_AIL, grp, CX_ufdma_appl2api_trace_lvl(global_lvl, ufdma_ail_trace_grps[grp].lvl));
        CX_netlink_trace_cfg_set(UFDMA_TRACE_LAYER_CIL, grp, CX_ufdma_appl2api_trace_lvl(global_lvl, ufdma_cil_trace_grps[grp].lvl));
    }
}

/******************************************************************************/
// packet_tx_props_init()
/******************************************************************************/
void packet_tx_props_init(packet_tx_props_t *tx_props)
{
    if (tx_props != NULL) {
        (void)mesa_packet_tx_info_init(NULL, &tx_props->tx_info);
        memset(&tx_props->packet_info, 0, sizeof(tx_props->packet_info));

        // Enforce modules to identify themselves.
        tx_props->packet_info.modid = VTSS_MODULE_ID_NONE;

        // Default to transmitting with highest priority.
        tx_props->tx_info.cos = msg_max_user_prio();
    }
}

/******************************************************************************/
// packet_tx()
/******************************************************************************/
mesa_rc packet_tx(packet_tx_props_t *tx_props)
{
    mesa_rc        rc;
    mesa_etype_t   saved_tpid;
    u32            port_cnt = 0;
    mesa_port_no_t port_no  = VTSS_PORT_NO_NONE;

    // Sanity checks:

    // tx_props must not be NULL
    PACKET_TX_CHECK(tx_props != NULL);

    // modid must be within range
    PACKET_TX_CHECK(tx_props->packet_info.modid <= VTSS_MODULE_ID_NONE);

    // Frame pointer must be specified
    PACKET_TX_CHECK(tx_props->packet_info.frm != NULL);

    if (!TX_masq_port) {
        // Masquerading not supported on all platforms
        PACKET_TX_CHECK(tx_props->tx_info.masquerade_port == VTSS_PORT_NO_NONE);
    }

    if (tx_props->tx_info.switch_frm) {

        // Super-prio injection not supported when switching frames.
        PACKET_TX_CHECK(tx_props->tx_info.cos < 8);

    } else {
        // Get info from the destination port mask.
        port_cnt = TX_mask_analyze(tx_props->tx_info.dst_port_mask, &port_no);

        // At least one bit must be set if not switching frame
        PACKET_TX_CHECK(port_cnt >= 1);

        // And if exactly one bit is set, it must be within valid port range.
        PACKET_TX_CHECK(port_cnt > 1 || port_no < CX_port_cnt);

        // Frames must not be sent masqueraded when not switching.
        PACKET_TX_CHECK(tx_props->tx_info.masquerade_port == VTSS_PORT_NO_NONE);

        if (tx_props->tx_info.cos == 8) {
            // With super-priority injection, frames must be directed towards a specific front port.
            PACKET_TX_CHECK(port_cnt == 1);
        }
    }

    // If the frame is sent switched without masquerading, then the VID must be non-zero unless masquerading, where it's optional.
    PACKET_TX_CHECK(tx_props->tx_info.masquerade_port != VTSS_PORT_NO_NONE || tx_props->tx_info.switch_frm == FALSE || (tx_props->tx_info.tag.vid & 0xFFF) != 0);

    // The QoS class must be in range [0; 8].
    PACKET_TX_CHECK(tx_props->tx_info.cos <= 8);

    // The length must be >= 14 bytes.
    PACKET_TX_CHECK(tx_props->packet_info.len >= 14);
    // Do not adjust the total length to become a minimum Ethernet-sized frame. This is done by the FDMA driver.

    PACKET_TX_CHECK(tx_props->tx_info.ptp_action < 16 && (TX_ptp_action & VTSS_BIT(tx_props->tx_info.ptp_action)) != 0);
    // #ptp_id must be in range [0-9]. Refer TS_IDS_RESERVED_FOR_SW in mesa.
    PACKET_TX_CHECK(tx_props->tx_info.ptp_id < 10);

    // #ptp_action != 0 not supported with #oam_type != 0.
    PACKET_TX_CHECK(tx_props->tx_info.oam_type == MESA_PACKET_OAM_TYPE_NONE || tx_props->tx_info.ptp_action == MESA_PACKET_PTP_ACTION_NONE);

    // Save a copy of user's tpid so that we haven't modified
    // the user's structure when this function returns
    saved_tpid = tx_props->tx_info.tag.tpid;

    if (tx_props->packet_info.filter.enable) {
        mesa_packet_port_info_t info;
        CapArray<mesa_packet_port_filter_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> filter;

        // Avoid Lint Warning 676: Possibly negative subscript (-1) in operator
        // Lint can't see that #port_no indeed is assigned when tx_info.switch_frm == FALSE
        /*lint --e{676} */

        // When Tx filtering is enabled, #switch_frm must be FALSE and we can only transmit to one single port at a time.
        PACKET_TX_CHECK(tx_props->tx_info.switch_frm == FALSE && port_cnt == 1);

        (void)mesa_packet_port_info_init(&info);
        info.port_no = tx_props->packet_info.filter.src_port;
        info.vid     = tx_props->tx_info.tag.vid;

        if (mesa_packet_port_filter_get(NULL, &info, filter.size(), filter.data()) != VTSS_RC_OK) {
            // Don't risk the mesa_packet_port_filter_get() function returning VTSS_RC_INV_STATE,
            // so take control of actual return value.
            return VTSS_RC_ERROR;
        }

        if (filter[port_no].filter == MESA_PACKET_FILTER_DISCARD) {
            return VTSS_RC_INV_STATE; // Special return value indicating that we're actually not sending the frame due to filtering.
        }

        // By setting TPID to a non-zero value, the FDMA driver will insert a VLAN tag according to tx_info.tag
        // into the frame prior to transmitting it.
        if (filter[port_no].filter == MESA_PACKET_FILTER_TAGGED) {
            tx_props->tx_info.tag.tpid = filter[port_no].tpid;
        } else {
            tx_props->tx_info.tag.vid = VTSS_VID_NULL; // In order not to get rewriter enabled.
        }
    }

    if (CX_stack_trace_ena || tx_props->packet_info.frm[ETYPE_POS] != 0x88 || tx_props->packet_info.frm[ETYPE_POS + 1] != 0x80) {
        T_DG(TRACE_GRP_TX, "Tx (%02x-%02x-%02x-%02x-%02x-%02x) by %s",
             tx_props->packet_info.frm[DMAC_POS + 0],
             tx_props->packet_info.frm[DMAC_POS + 1],
             tx_props->packet_info.frm[DMAC_POS + 2],
             tx_props->packet_info.frm[DMAC_POS + 3],
             tx_props->packet_info.frm[DMAC_POS + 4],
             tx_props->packet_info.frm[DMAC_POS + 5],
             vtss_module_names[tx_props->packet_info.modid]);
    }

    rc = TX_npi(tx_props);

    if (rc == VTSS_RC_OK) {
        packet_module_counters_t *cntrs = &CX_module_counters[tx_props->packet_info.modid];

        PACKET_CX_COUNTER_CRIT_ENTER();
        CX_port_counters.tx_pkts[tx_props->tx_info.switch_frm ? CX_port_cnt : port_cnt > 1 ? CX_port_cnt + 1 : port_no]++;
        cntrs->tx_bytes += tx_props->packet_info.len;
        cntrs->tx_pkts++;
        PACKET_CX_COUNTER_CRIT_EXIT();
    }

    // Restore the user's src_port before exiting.
    tx_props->tx_info.tag.tpid = saved_tpid;
    return rc;
}

/******************************************************************************/
// packet_tx_alloc()
// Size argument should not include IFH and FCS
/******************************************************************************/
u8 *packet_tx_alloc(size_t size)
{
    u8 *buffer;

    size = MAX(60, size) /* minimum-sized Ethernet frame excl. FCS */ + 4 /* possible VLAN tag */;

    if ((buffer = (u8 *)VTSS_MALLOC(size))) {
        buffer += 4; /* possible VLAN tag added during TX_npi() */
        PACKET_CX_COUNTER_CRIT_ENTER();
        TX_alloc_calls++;
        PACKET_CX_COUNTER_CRIT_EXIT();
    }

    return buffer;
}

/******************************************************************************/
// packet_tx_alloc_extra()
// The difference between this function and the packet_tx_alloc() function is
// that the user is able to reserve a number of 32-bit words at the beginning
// of the packet, which is useful when some state must be saved between the call
// to the packet_tx() function and the callback function.
// Args:
//   @size              : Size exluding IFH and FCS
//   @extra_size_dwords : Number of 32-bit words to reserve room for.
//   @extra_ptr         : Pointer that after the call will contain the pointer to the additional space.
// Returns:
//   Pointer to the location that the DMAC should be stored. If function fails,
//   the function returns NULL.
// Use packet_tx_free_extra() when freeing the packet rather than packet_tx_free().
/******************************************************************************/
u8 *packet_tx_alloc_extra(size_t size, size_t extra_size_dwords, u8 **extra_ptr)
{
    u8 *buffer;
    // The user must call us with the number of 32-bit words he wants extra or
    // we might end up with an IFH that is not 32-bit aligned.
    size_t extra_size_bytes = 4 * extra_size_dwords;

    PACKET_CHECK(extra_ptr, return NULL;);
    size = MAX(60, size) /* minimum-sized Ethernet frame excl. FCS */ + 4 /* possible VLAN tag */;

    if ((buffer = (u8 *)VTSS_MALLOC(size + extra_size_bytes))) {
        *extra_ptr = buffer;
        buffer    += extra_size_bytes + 4 /* possible VLAN tag */;
        PACKET_CX_COUNTER_CRIT_ENTER();
        TX_alloc_calls++;
        PACKET_CX_COUNTER_CRIT_EXIT();
    }

    return buffer;
}

/******************************************************************************/
// packet_tx_free()
/******************************************************************************/
void packet_tx_free(u8 *buffer)
{
    PACKET_CHECK(buffer != NULL, return;);
    buffer -= 4 /* possible VLAN tag */;
    VTSS_FREE(buffer);
    PACKET_CX_COUNTER_CRIT_ENTER();
    TX_free_calls++;
    PACKET_CX_COUNTER_CRIT_EXIT();
}

/******************************************************************************/
// packet_tx_free_extra()
// This function is the counter-part to the packet_tx_alloc_extra() function.
// It must be called with the value returned in packet_tx_alloc_extra()'s
// extra_ptr argument.
/******************************************************************************/
void packet_tx_free_extra(u8 *extra_ptr)
{
    PACKET_CHECK(extra_ptr != NULL, return;);
    VTSS_FREE(extra_ptr);
    PACKET_CX_COUNTER_CRIT_ENTER();
    TX_free_calls++;
    PACKET_CX_COUNTER_CRIT_EXIT();
}

/******************************************************************************/
// packet_rx_filter_init()
/******************************************************************************/
void packet_rx_filter_init(packet_rx_filter_t *filter)
{
    if (!filter) {
        T_E("filter is NULL");
        return;
    }

    memset(filter, 0, sizeof(*filter));
    filter->thread_prio = VTSS_THREAD_PRIO_DEFAULT;
    filter->mtu         = PACKET_RX_MTU_DEFAULT;
}

/******************************************************************************/
// packet_rx_filter_register()
// Context: Thread only
/******************************************************************************/
mesa_rc packet_rx_filter_register(const packet_rx_filter_t *filter, void **filter_id)
{
    mesa_rc rc = PACKET_RC_PARAM;

    if (!filter_id) {
        T_E("filter_id must not be a NULL pointer");
        return rc;
    }

    // Validate the filter.
    if (!RX_filter_validate(filter)) {
        return rc;
    }

    PACKET_RX_FILTER_CRIT_ENTER();
    if (!RX_filter_insert(filter, filter_id)) {
        goto exit_func;
    }

    rc = VTSS_RC_OK;
exit_func:
    PACKET_RX_FILTER_CRIT_EXIT();
    return rc;
}

/******************************************************************************/
// packet_rx_filter_unregister()
// Context: Thread only
// Remove a subscription.
/******************************************************************************/
mesa_rc packet_rx_filter_unregister(void *filter_id)
{
    mesa_rc rc = PACKET_RC_GEN;

    PACKET_RX_FILTER_CRIT_ENTER();

    if (!RX_filter_remove(filter_id)) {
        goto exit_func;
    }

    rc = VTSS_RC_OK;

exit_func:
    PACKET_RX_FILTER_CRIT_EXIT();
    return rc;
}

/******************************************************************************/
// packet_rx_filter_change()
// Context: Thread only
// The change function allows a subscriber to change his filter on the
// fly. This is useful for e.g. the 802.1X protocol which just needs to
// update the source mask now and then as ports get in and out of
// authentication.
// If an error occurs, the current subscription is not changed.
// If an error doesn't occur, the filter_id may change.
// The call of this function corresponds to atomar calls to unregister()
// and register().
/******************************************************************************/
mesa_rc packet_rx_filter_change(const packet_rx_filter_t *filter, void **filter_id)
{
    mesa_rc rc = PACKET_RC_GEN;

    if (!filter_id) {
        T_E("filter_id must not be a NULL pointer");
        return rc;
    }

    if (!RX_filter_validate(filter)) {
        return rc;
    }

    PACKET_RX_FILTER_CRIT_ENTER();

    // Unplug the current filter ID
    // The reason for not just changing the current filter
    // is that the user may have changed priority, so that
    // it must be moved from one position to another in the
    // list, which is easily handled by first removing, then
    // inserting again.
    if (!RX_filter_remove(*filter_id)) {
        goto exit_func;
    }

    if (!RX_filter_insert(filter, filter_id)) {
        goto exit_func;
    }

    rc = VTSS_RC_OK;

exit_func:
    PACKET_RX_FILTER_CRIT_EXIT();
    return rc;
}

/****************************************************************************/
// packet_uninit()
/****************************************************************************/
void packet_uninit(void)
{
}

/******************************************************************************/
// packet_rx_shaping_cfg_get()
// Function only intended to be used by CLI debug
/******************************************************************************/
mesa_rc packet_rx_shaping_cfg_get(u32 *l1_rate_kbps)
{
    mesa_packet_rx_conf_t rx_conf;
    mesa_rc               rc;

    if (!l1_rate_kbps) {
        return VTSS_RC_ERROR;
    }

    // Get Rx packet configuration */
    if ((rc = mesa_packet_rx_conf_get(0, &rx_conf)) != VTSS_RC_OK) {
        return rc;
    }

    // VTSS_BITRATE_DISABLED means disabled towards the API, whereas we use 0 to indicate this.
    *l1_rate_kbps = rx_conf.shaper_rate == VTSS_BITRATE_DISABLED ? 0 : rx_conf.shaper_rate;

    return VTSS_RC_OK;
}

/******************************************************************************/
// packet_rx_shaping_cfg_set()
// Function only intended to be used by CLI debug
/******************************************************************************/
mesa_rc packet_rx_shaping_cfg_set(u32 l1_rate_kbps)
{
    mesa_packet_rx_conf_t rx_conf;
    mesa_rc               rc;

    // mesa_packet_rx_conf_get()/set() must be called without interference.
    VTSS_APPL_API_LOCK_SCOPE();

    // Get Rx packet configuration */
    if ((rc = mesa_packet_rx_conf_get(0, &rx_conf)) != VTSS_RC_OK) {
        return rc;
    }

    if (l1_rate_kbps == 0) {
        // Our interface uses 0 to disable the shaper,
        // whereas the API uses VTSS_BITRATE_DISABLED.
        l1_rate_kbps = VTSS_BITRATE_DISABLED;
    }

    rx_conf.shaper_rate = l1_rate_kbps;

    // Set Rx packet configuration */
    rc = mesa_packet_rx_conf_set(0, &rx_conf);

    return rc;
}

/******************************************************************************/
// packet_rx_throttle_cfg_get()
/******************************************************************************/
mesa_rc packet_rx_throttle_cfg_get(packet_throttle_cfg_t *packet_throttle_cfg)
{
    if (fast_cap(MESA_CAP_QOS_CPU_QUEUE_SHAPER)) {
        // Platform has H/W support. Use that.
        T_I("Using MESA for CPU queue throttling");
        return RX_throttle_cfg_mesa_get(packet_throttle_cfg);
    } else {
        if (CX_internal_mips) {
            // No H/W support, so utilize uFDMA support
            T_I("Using FDMA + Netlink for CPU queue throttling");
            return RX_throttle_cfg_netlink_get(packet_throttle_cfg);
        } else {
            // External CPU or not a MIPS without CPU queue shaper H/W support
            T_I("Packet Rx throttling not supported");
            return PACKET_RC_THROTTLING_NOT_SUPPORTED;
        }
    }
}

/******************************************************************************/
// packet_rx_throttle_cfg_set()
/******************************************************************************/
mesa_rc packet_rx_throttle_cfg_set(const packet_throttle_cfg_t *packet_throttle_cfg)
{
    if (fast_cap(MESA_CAP_QOS_CPU_QUEUE_SHAPER)) {
        // Platform has H/W support. Use that.
        T_I("Using MESA for CPU queue throttling");
        return RX_throttle_cfg_mesa_set(packet_throttle_cfg);
    } else {
        if (CX_internal_mips) {
            // No H/W support, so utilize uFDMA support
            T_I("Using FDMA + Netlink for CPU queue throttling");
            return RX_throttle_cfg_netlink_set(packet_throttle_cfg);
        } else {
            // External CPU or not a MIPS without CPU queue shaper H/W support
            T_I("Packet Rx throttling not supported");
            return PACKET_RC_THROTTLING_NOT_SUPPORTED;
        }
    }
}

/******************************************************************************/
// packet_rx_queue_usage()
/******************************************************************************/
char *packet_rx_queue_usage(u32 xtr_qu, char *buf, size_t size)
{
    int      cnt  = 0;
    uint32_t mask = VTSS_BIT(xtr_qu);

    if (!buf || size == 0) {
        return NULL;
    }

    buf[0] = '\0';

    if (xtr_qu >= VTSS_PACKET_RX_QUEUE_CNT) {
        cnt += snprintf(buf + cnt, MAX((int)size - cnt, 0), "Error: Invalid extraction queue (%u = 0x%x)", xtr_qu, xtr_qu);
        return buf;
    }

    // What we don't do in order to make Coverity shut up, because several of
    // the PACKET_XTR_QU_xxx are identical.
    mask = VTSS_BIT(xtr_qu);
    if (mask & VTSS_BIT(PACKET_XTR_QU_BPDU)) {
        cnt += snprintf(buf + cnt, MAX((int)size - cnt, 0), "BPDUs");
    }

    if (mask & VTSS_BIT(PACKET_XTR_QU_IGMP)) {
        cnt += snprintf(buf + cnt, MAX((int)size - cnt, 0), "%sIPMC", cnt != 0 ? ", " : "");
    }

    if (mask & VTSS_BIT(PACKET_XTR_QU_MGMT_MAC)) {
        cnt += snprintf(buf + cnt, MAX((int)size - cnt, 0), "%sanagement DMAC", cnt != 0 ? ", m" : "M");
    }

    if (mask & VTSS_BIT(PACKET_XTR_QU_L3_OTHER)) {
        cnt += snprintf(buf + cnt, MAX((int)size - cnt, 0), "%sanagement DMAC w/ TTL expiration or IP options", cnt != 0 ? ", m" : "M");
    }

    if (mask & VTSS_BIT(PACKET_XTR_QU_REDBOX)) {
        cnt += snprintf(buf + cnt, MAX((int)size - cnt, 0), "RedBox frames");
    }

    if (mask & VTSS_BIT(PACKET_XTR_QU_MAC)) {
        cnt += snprintf(buf + cnt, MAX((int)size - cnt, 0), "%sort security", cnt != 0 ? ", p" : "P");
    }

    if (mask & VTSS_BIT(PACKET_XTR_QU_OAM)) {
        cnt += snprintf(buf + cnt, MAX((int)size - cnt, 0), "%sOAM", cnt != 0 ? ", " : "");
    }

    if (mask & VTSS_BIT(PACKET_XTR_QU_L2CP)) {
        cnt += snprintf(buf + cnt, MAX((int)size - cnt, 0), "%sL2CP", cnt != 0 ? ", " : "");
    }

    if (mask & VTSS_BIT(PACKET_XTR_QU_BC)) {
        cnt += snprintf(buf + cnt, MAX((int)size - cnt, 0), "%sroadcast", cnt != 0 ? ", b" : "B");
    }

    if (mask & VTSS_BIT(PACKET_XTR_QU_LEARN)) {
        cnt += snprintf(buf + cnt, MAX((int)size - cnt, 0), "%secure learning", cnt != 0 ? ", s" : "S");
    }

    if (mask & VTSS_BIT(PACKET_XTR_QU_ACL_COPY)) {
        cnt += snprintf(buf + cnt, MAX((int)size - cnt, 0), "%sACL Copy", cnt != 0 ? ", " : "");
    }

    if (mask & VTSS_BIT(PACKET_XTR_QU_ACL_REDIR)) {
        cnt += snprintf(buf + cnt, MAX((int)size - cnt, 0), "%sACL Redirect", cnt != 0 ? ", " : "");
    }

    if (mask & VTSS_BIT(PACKET_XTR_QU_SFLOW)) {
        cnt += snprintf(buf + cnt, MAX((int)size - cnt, 0), "%ssFlow", cnt != 0 ? ", " : "");
    }

    if (mask & VTSS_BIT(PACKET_XTR_QU_LRN_ALL)) {
        cnt += snprintf(buf + cnt, MAX((int)size - cnt, 0), "%sLearn-All", cnt != 0 ? ", " : "");
    }

    if (cnt == 0) {
        (void)snprintf(buf, size, "<none>");
    }

    return buf;
}

/******************************************************************************/
/******************************************************************************/
typedef enum {
    PACKET_DBG_CMD_STAT_PACKET_PRINT       =  1,
    PACKET_DBG_CMD_STAT_FDMA_PRINT,
    PACKET_DBG_CMD_STAT_PORT_PRINT,
    PACKET_DBG_CMD_SUBSCRIBERS_PRINT,
    PACKET_DBG_CMD_STAT_THREAD_PRINT,
    PACKET_DBG_CMD_STAT_RX_QU_PRINT,
    PACKET_DBG_CMD_STAT_PACKET_CLEAR       = 10,
    PACKET_DBG_CMD_STAT_FDMA_CLEAR,
    PACKET_DBG_CMD_STAT_PORT_CLEAR,
    PACKET_DBG_CMD_STAT_THREAD_CLEAR,
    PACKET_DBG_CMD_STAT_RX_QU_CLEAR,
    PACKET_DBG_CMD_STAT_ALL_CLEAR          = 19,
    PACKET_DBG_CMD_CFG_STACK_TRACE         = 20,
    PACKET_DBG_CMD_CFG_SIGNAL_TX_PEND_COND = 22,
    PACKET_DBG_CMD_TEST_SYSLOG             = 40,
} packet_dbg_cmd_num_t;

/******************************************************************************/
/******************************************************************************/
typedef struct {
    packet_dbg_cmd_num_t cmd_num;
    const char           *cmd_txt;
    const char           *arg_syntax;
    uint                 max_arg_cnt;
    void                 (*func)(packet_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms);
} packet_dbg_cmd_t;

/******************************************************************************/
/******************************************************************************/
static const packet_dbg_cmd_t packet_dbg_cmds[] = {
    {
        PACKET_DBG_CMD_STAT_PACKET_PRINT,
        "Print per-module statistics",
        NULL,
        0,
        DBG_cmd_stat_module_print
    },
    {
        PACKET_DBG_CMD_STAT_FDMA_PRINT,
        "Print FDMA statistics",
        NULL,
        0,
        DBG_cmd_stat_fdma_print
    },
    {
        PACKET_DBG_CMD_STAT_PORT_PRINT,
        "Print per-port statistics",
        NULL,
        0,
        DBG_cmd_stat_port_print
    },
    {
        PACKET_DBG_CMD_SUBSCRIBERS_PRINT,
        "Print subscriber list",
        NULL,
        0,
        DBG_cmd_subscribers_print
    },
    {
        PACKET_DBG_CMD_STAT_THREAD_PRINT,
        "Print per-Rx-thread statistics",
        NULL,
        0,
        DBG_cmd_stat_thread_print
    },
    {
        PACKET_DBG_CMD_STAT_RX_QU_PRINT,
        "Print Rx queue statistics",
        NULL,
        0,
        DBG_cmd_stat_rx_qu_print
    },
    {
        PACKET_DBG_CMD_STAT_PACKET_CLEAR,
        "Clear per-module statistics",
        NULL,
        0,
        DBG_cmd_stat_packet_clear
    },
    {
        PACKET_DBG_CMD_STAT_PORT_CLEAR,
        "Clear port statistics",
        NULL,
        0,
        DBG_cmd_stat_port_clear
    },
    {
        PACKET_DBG_CMD_STAT_THREAD_CLEAR,
        "Clear thread statistics",
        NULL,
        0,
        DBG_cmd_stat_thread_clear
    },
    {
        PACKET_DBG_CMD_STAT_RX_QU_CLEAR,
        "Clear extraction queue statistics",
        NULL,
        0,
        DBG_cmd_stat_rx_qu_clear
    },
    {
        PACKET_DBG_CMD_STAT_ALL_CLEAR,
        "Clear all statistics (including uFDMA if applicable)",
        NULL,
        0,
        DBG_cmd_stat_all_clear
    },
    {
        PACKET_DBG_CMD_CFG_STACK_TRACE,
        "Enable or disable stack trace",
        "0: Disable, 1: Enable",
        1,
        DBG_cmd_cfg_stack_trace
    },
    {
        PACKET_DBG_CMD_TEST_SYSLOG,
        "Generate error or fatal. This is only to test the SYSLOG, and has nothing to do with the packet module",
        "0: Generate error, 1: Generate assert (never returns)",
        1,
        DBG_cmd_test_syslog
    },
    {
        (packet_dbg_cmd_num_t)0,
        NULL,
        NULL,
        0,
        NULL
    }
};

/******************************************************************************/
// packet_dbg()
/******************************************************************************/
void packet_dbg(packet_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    int i;
    u32 cmd_num;

    if (parms_cnt == 0) {
        (void)dbg_printf("Usage: debug packet <cmd idx>\n");
        (void)dbg_printf("Most commands show current settings if called without arguments\n\n");
        (void)dbg_printf("Commands:\n");
        i = 0;

        while (packet_dbg_cmds[i].cmd_num != 0) {
            (void)dbg_printf("  %2d: %s\n", packet_dbg_cmds[i].cmd_num, packet_dbg_cmds[i].cmd_txt);
            if (packet_dbg_cmds[i].arg_syntax && packet_dbg_cmds[i].arg_syntax[0]) {
                (void)dbg_printf("      Arguments: %s.\n", packet_dbg_cmds[i].arg_syntax);
            }
            i++;
        }
        return;
    }

    cmd_num = parms[0];

    // Verify that command is known and argument count is correct
    i = 0;
    while (packet_dbg_cmds[i].cmd_num != 0) {
        if (packet_dbg_cmds[i].cmd_num == cmd_num) {
            break;
        }
        i++;
    }

    if (packet_dbg_cmds[i].cmd_num == 0) {
        DBG_cmd_syntax_error(dbg_printf, "Unknown command number: %d", cmd_num);
        return;
    }

    if (parms_cnt - 1 > packet_dbg_cmds[i].max_arg_cnt) {
        DBG_cmd_syntax_error(dbg_printf, "Incorrect number of arguments (%d).\n"
                             "Arguments: %s",
                             parms_cnt - 1,
                             packet_dbg_cmds[i].arg_syntax);
        return;
    }

    if (packet_dbg_cmds[i].func == NULL) {
        (void)dbg_printf("Internal Error: Function for command %u not implemented (yet?)", cmd_num);
        return;
    }

    packet_dbg_cmds[i].func(dbg_printf, parms_cnt - 1, parms + 1);
}

/******************************************************************************/
// mesa_vlan_tag_t::operator<<()
// Used for tracing
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_vlan_tag_t &t)
{
    o << "{tpid = 0x" << vtss::hex(t.tpid)
      << ", pcp = "   << t.pcp
      << ", dei = "   << t.dei
      << ", vid = "   << t.vid
      << "}";

    return o;
}

/******************************************************************************/
// mesa_packet_tx_info_t::operator<<()
// Used for tracing
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_packet_tx_info_t &t)
{
    char buf[20], buf2[20], buf3[20];

    sprintf(buf, VPRI64Fx("016"), t.dst_port_mask);

    if (t.masquerade_port == VTSS_PORT_NO_NONE) {
        strcpy(buf2, "None");
    } else {
        sprintf(buf2, "%u", t.masquerade_port);
    }

    if (t.afi_id == MESA_AFI_ID_NONE) {
        strcpy(buf3, "None");
    } else {
        sprintf(buf3, "%u", t.afi_id);
    }

    o << "{switch_frm = "            << t.switch_frm
      << ", dst_port_mask = "        << buf
      << ", tag = "                  << t.tag // Using mesa_vlan_tag_t::operator<<()
      << ", cos = "                  << t.cos
      << ", cosid = "                << t.cosid
      << ", dp = "                   << t.dp
      << ", ptp_action = "           << TX_ptp_action_to_str(t.ptp_action) << " (" << t.ptp_action << ")"
      << ", ptp_id = "               << t.ptp_id
      << ", ptp_timestamp = "        << t.ptp_timestamp
      << ", oam_type = "             << TX_oam_type_to_str(t.oam_type) << " (" << t.oam_type << ")"
      << ", iflow_id = "             << t.iflow_id
      << ", masquerade_port = "      << buf2
      << ", pdu_offset = "           << t.pdu_offset
      << ", afi_id = "               << buf3
      << ", pipeline_pt = "          << TX_pipeline_pt_to_str(t.pipeline_pt) << " (" << t.pipeline_pt << ")"
      << ", rb_tag_disable = "       << t.rb_tag_disable
      << ", rb_dd_disable = "        << t.rb_dd_disable
      << ", rb_fwd = "               << TX_rb_fwd_to_str(t.rb_fwd) << " (" << t.rb_fwd << ")"
      << ", rb_ring_netid_enable = " << t.rb_ring_netid_enable
      << "}";

    return o;
}

/******************************************************************************/
// packet_tx_info_t::operator<<()
// Used for tracing
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const packet_tx_info_t &t)
{
    o << "{modid = "    << vtss_module_names[t.modid] << " (" << t.modid << ")"
      << ", frm = "     << t.frm
      << ", len = "     << t.len
      << ", no_free = " << t.no_free
      << "}";

    return o;
}

/******************************************************************************/
// packet_tx_props_t::operator<<()
// Used for tracing
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const packet_tx_props_t &props)
{
    o << "{tx_info = "      << props.tx_info     // Using mesa_packet_tx_info_t::operator<<()
      << ", packet_info = " << props.packet_info // Using packet_tx_info_t::operator<<()
      << "}";

    return o;
}

/******************************************************************************/
// packet_debug_tx_props_print()
/******************************************************************************/
void packet_debug_tx_props_print(vtss_module_id_t trace_mod, u32 trace_grp, u32 trace_lvl, const packet_tx_props_t *props)
{
    if (!TRACE_IS_ENABLED(trace_mod, trace_grp, trace_lvl)) {
        return;
    }

    T_MOD(trace_mod, trace_grp, trace_lvl, "%s", *props); // Using fmt(packet_tx_props_t::operator<<()
}

/******************************************************************************/
// RX_hints_to_str()
/******************************************************************************/
static char *RX_hints_to_str(char *buf, size_t size, uint32_t hints)
{
    int  s, res;
    bool first;

    if (!buf) {
        return buf;
    }

#define P(_str_)                                        \
    if (size - s > 0) {                                 \
        res = snprintf(buf + s, size - s, "%s", _str_); \
        if (res > 0) {                                  \
            s += res;                                   \
        }                                               \
    }

#define F(X, _name_)                                         \
    if (hints & MESA_PACKET_RX_HINTS_##X) { \
        hints &= ~MESA_PACKET_RX_HINTS_##X; \
        if (first) {                                         \
            first = false;                                   \
            P(_name_);                                       \
        } else {                                             \
            P(", " _name_);                                  \
        }                                                    \
    }

    buf[0] = 0;
    s      = 0;
    first  = true;

    // Example of a field name (just so that we can search for this function):
    // MESA_PACKET_RX_HINTS_VLAN_TAG_MISMATCH

    F(VLAN_TAG_MISMATCH,                          "VLAN tag mismatch");
    F(VLAN_FRAME_MISMATCH,                        "VLAN frame mismatch");
    F(VID_MISMATCH,                               "VID mismatch");
#undef F
#undef P

    if (hints != 0) {
        T_E("Not all hints are handled. Missing = 0x%x", hints);
    }

    return buf;
}

/******************************************************************************/
// mesa_packet_rx_info_t::operator<<()
// Used for tracing
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_packet_rx_info_t &info)
{
    char hints_str[100], if_str[40];

    o << "{hints = "          << RX_hints_to_str(hints_str, sizeof(hints_str), info.hints)
      << ", length = "        << info.length
      << ", port_no = "       << info.port_no << " (" << icli_port_info_txt_short(VTSS_USID_START, iport2uport(info.port_no), if_str) << ")"
      << ", tag_type = "      << RX_tag_type_to_str(info.tag_type)
      << ", tag = "           << info.tag          // Using mesa_vlan_tag_t::operator<<()
      << ", stripped_tag = "  << info.stripped_tag // Using mesa_vlan_tag_t::operator<<()
      << ", xtr_qu_mask = "   << "0x" << vtss::hex(info.xtr_qu_mask)
      << ", cos = "           << info.cos
      << ", cosid = "         << info.cosid
      << ", dp = "            << info.dp
      << ", acl_hit = "       << info.acl_hit;

    if (info.tstamp_id_decoded) {
        o << ", tstamp_id = " << info.tstamp_id;
    }

    if (info.hw_tstamp_decoded) {
        o << ", hw_tstamp = " << info.hw_tstamp;
    }

    o << ", sflow_type = "    << RX_sflow_type_to_str(info.sflow_type);

    if (info.sflow_type != MESA_SFLOW_TYPE_NONE) {
        o << ", sflow_port_no = " << info.sflow_port_no << " (" << icli_port_info_txt_short(VTSS_USID_START, iport2uport(info.sflow_port_no), if_str) << ")";
    }

    o << ", iflow_id = "      << info.iflow_id
      << ", rb_port_a = "     << info.rb_port_a
      << ", rb_tagged = "     << info.rb_tagged
      << "}";

    return o;
}

extern "C" int packet_icli_cmd_register();

/******************************************************************************/
// Initialize packet module
/******************************************************************************/
mesa_rc packet_init(vtss_init_data_t *data)
{
    int      i;
    uint32_t chip_family;
    std::string outstr, errstr;

    if (data->cmd == INIT_CMD_EARLY_INIT) {
        // Initialize "vtss.ifh"
        CX_npi_init();

        CX_port_cnt = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);
        CX_internal_cpu = !misc_cpu_is_external();

        chip_family = fast_cap(MESA_CAP_MISC_CHIP_FAMILY);
        CX_internal_mips = CX_internal_cpu && chip_family != MESA_CHIP_FAMILY_SPARX5 && chip_family != MESA_CHIP_FAMILY_LAN966X && chip_family != MESA_CHIP_FAMILY_LAN969X;

        // SparX5 and LAN969x don't support the dst_port_mask field in the IFH.
        TX_ifh_has_dst_port_mask = chip_family != MESA_CHIP_FAMILY_SPARX5 && chip_family != MESA_CHIP_FAMILY_LAN969X;

        // Only applicable for iCPU
        if (CX_internal_cpu) {
            // Initialize and register uFDMA CIL trace resources (the AIL are
            // registered statically already).
            for (i = 0; i < ARRSZ(ufdma_cil_trace_grps); i++) {
                ufdma_cil_trace_grps[i] = ufdma_ail_trace_grps[i];
                ufdma_cil_trace_grps[i].flags &= ~VTSS_TRACE_FLAGS_INIT;
            }

            // CIL
            vtss_trace_reg_init(&ufdma_cil_trace_reg, ufdma_cil_trace_grps, ARRSZ(ufdma_cil_trace_grps));
            vtss_trace_register(&ufdma_cil_trace_reg);
        }
    } else if (data->cmd == INIT_CMD_INIT) {
        packet_icli_cmd_register();

        if (CX_internal_cpu) {
            // Update uFDMA trace levels to initialization settings
            packet_ufdma_trace_update();
        }

        vtss_clear(CX_port_counters);
        memset(CX_module_counters, 0, sizeof(CX_module_counters));

        // The purpose of the following sysctls is to limit the amount of memory
        // spent on IP fragments. The ipfrag_time is set to 1 second so that if
        // all fragments belonging to an IP packet doesn't arrive within 1 second,
        // then the fragment queue is deleted. This will hand back the Rx buffers
        // to the FDMA.
        // 1 second is chosen because that should be long enough for a 64 Kbyte
        // IP-fragmented segment to arrive.
        // The 64K and 128K thresholds are chosen because it allows for being
        // able to respond to 64K pings (ping <ip_addr> -s 65507) simultaneously,
        // or for receiving large IP-fragmented UDP frames.
        //
        // These settings also help against the New Dawn attack (see ../test/new_dawn.c).
        // The thresholds are measured in "truesize". The New Dawn attack sends small
        // fragments (64-byte frames), which will yield a truesize of 176 bytes (don't
        // know why this diff). With 256 Rx buffers in the kernel, we can therefore
        // only reach 256 * 176 = 45056 truesize bytes, which is smaller than the
        // threshold we are setting up below. Therefore, with 256 Rx buffers, we will
        // always hit the timeout before hitting the threshold.
        // In order to hit the threshold, we must have at least 131072 / 176 = 745 Rx
        // buffers. Having this many, will cause us to reply to legitimate traffic
        // much faster.
        //
        // Defaults in the Linux kernel are:
        // Low  = 3 MBytes
        // High = 4 MBytes
        // Time = 30 seconds
        //
        // There does not seem to be any significant difference in
        // behavior whether ipfrag_time is set to 1 or left at default
        // 30. Therefore, revert to default value
        CX_sysctl_set("/proc/sys/net/ipv4/ipfrag_low_thresh", 65536);
        CX_sysctl_set("/proc/sys/net/ipv4/ipfrag_high_thresh", 131072);

        // Set maximum number of bytes that a socket can have outstanding on the
        // Rx side to something very high, so that we can take advantage of this
        // once we open vtss.ifh. See also CX_socket_rcvbuf_set().
        system("sysctl -w net.core.rmem_max=20000000 > /dev/null");

        // Initialize injection part
        TX_init();

        // Initialize extraction part
        RX_init();

        // NPI encapsulation EPID
        npi_encap[NPI_ENCAP_LEN - 1] = fast_cap(MESA_CAP_PACKET_IFH_EPID);
    } else if (data->cmd == INIT_CMD_START) {

        RX_s_custom_etype_change_hook();
        RX_throttle_init();

        if (CX_internal_cpu) {
            // Only applicable for iCPU
            RX_tcp_rx_win_set();
        }

        if (fast_cap(MESA_CAP_QOS_CPU_PORT_SHAPER)) {
            RX_shaping_init();
        }

        RX_conf_setup();
    }

    return VTSS_RC_OK;
}

