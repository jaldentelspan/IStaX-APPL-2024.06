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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#include "msg.h"
#include "msg_api.h"
#include "port_api.h" /* For port_count_max() */
#include "misc_api.h"
#include "lock.hxx"   /* For vtss::Lock */
#include "vtss_os_wrapper_network.h"
#include "packet_api.h"

#define MSG_INLINE inline
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_MSG

/*lint -esym(459, DBG_cmd_cfg_trace_isid_set)  */
/*lint -esym(459, DBG_cmd_cfg_trace_modid_set) */
/*lint -esym(459, DBG_cmd_test_run)            */
/*lint -esym(457, TX_thread)                   */
/*lint -esym(459, this_mac)                    */

/****************************************************************************/
// Trace definitions
/****************************************************************************/
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_MSG
#define VTSS_TRACE_GRP_DEFAULT  0
#define TRACE_GRP_CFG           1
#define TRACE_GRP_RX            2
#define TRACE_GRP_TX            3
#define TRACE_GRP_RELAY         4
#define TRACE_GRP_INIT_MODULES  5
#define TRACE_GRP_TOPO          6
#define TRACE_GRP_CALLBACK      7
#define TRACE_GRP_WAIT          8
#include <vtss_trace_api.h>

/* Trace registration. Initialized by msg_init() */
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "msg", "Message module"
};

#ifndef MSG_DEFAULT_TRACE_LVL
#define MSG_DEFAULT_TRACE_LVL VTSS_TRACE_LVL_ERROR
#endif

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        MSG_DEFAULT_TRACE_LVL
    },
    [TRACE_GRP_CFG] = {
        "cfg",
        "Configuration",
        MSG_DEFAULT_TRACE_LVL
    },
    [TRACE_GRP_RX] = {
        // Level Usage:
        //   FAILURE:
        //     Internal fatal error (can never be asserted as the result of a malformed received frame)
        //   ERROR:
        //     The received frame doesn't conform to the message protocol standard
        //   WARNING:
        //     If e.g. receiving an unexpected message protocol frame.
        //   INFO:
        //     Show one-liner when a message gets dispatched.
        //   DEBUG:
        //     Show one-liner when a frame is received and other info
        //   NOISE:
        //     Show message contents when dispatching.
        //   RACKET:
        //     Show frame contents of all received frames destined to this switch.
        "rx",
        "Rx",
        MSG_DEFAULT_TRACE_LVL
    },
    [TRACE_GRP_TX] = {
        // Level Usage:
        //   FAILURE:
        //     Internal fatal error
        //   ERROR:
        //     Not used.
        //   WARNING:
        //     If e.g. retransmitting an MD.
        //   INFO:
        //     Show user calls to msg_tx() and when user's tx_done is called back.
        //   DEBUG:
        //     Show one-liner of transmitted frames
        //   NOISE:
        //     Show user's message when he calls msg_tx() and when user's tx_done is called back
        //   RACKET:
        //     Show frame contents of all transmitted frames.
        "tx",
        "Tx",
        MSG_DEFAULT_TRACE_LVL
    },
    [TRACE_GRP_RELAY] = {
        "relay",
        "Relay",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_INIT_MODULES] = {
        "initmods",
        "init_modules() calls",
        MSG_DEFAULT_TRACE_LVL
    },
    [TRACE_GRP_TOPO] = {
        "topo",
        "Events from Topo",
        MSG_DEFAULT_TRACE_LVL
    },
    [TRACE_GRP_CALLBACK] = {
        "callback",
        "Rx module callback",
        MSG_DEFAULT_TRACE_LVL
    },
    [TRACE_GRP_WAIT] = {
        "wait",
        "Waiters for events",
        MSG_DEFAULT_TRACE_LVL
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

// Assert with return value. Useful if compiled without assertions.
#define MSG_ASSERTR(expr, fmt, ...) {         \
    if (!(expr)) {                            \
        MSG_ASSERT(expr, fmt, ##__VA_ARGS__); \
        return MSG_ASSERT_FAILURE;            \
    }                                         \
}

// Synchronization primitive that gets unlocked just before the INIT_CMD_ICFG_LOADING_PRE
// event gets propagated to all modules and remains unlocked until we're no longer
// master.
static vtss::Lock CX_master_up_pre_lock(true);

// Synchronization primitive that is locked as long as the INIT_CMD_ICFG_LOADING_PRE event
// has not been propagated through all modules and is only open while we're master.
static vtss::Lock CX_master_up_post_lock(true);

// Synchronization primitive that is locked as long as the
// INIT_CMD_ICFG_LOADING_POST event has not been propagated through all modules.
static vtss::Lock CX_switch_add_post_lock(true);

// Synchronization primitive that is locked during boot, until the INIT_CMD_INIT
// event has been propagated through all modules.
// This really belongs to (linux_)main.c, but due to the similarity
// with the CX_master_up_pre_lock() and CX_master_up_post_lock(), we keep it here.
static vtss::Lock CX_init_done_lock(true);

// Lock that prevents RX_thread to run before TX_thread tells it to
static vtss::Lock RX_thread_lock(true);

// Allow limitation of trace output per ISID when master.
static BOOL msg_trace_enabled_per_isid[VTSS_ISID_CNT + 1];

// Allow limitation of trace output per Module ID when master or slave
static BOOL msg_trace_enabled_per_modid[VTSS_MODULE_ID_NONE + 1];

// Shape messages subject to shaping.
static struct {
    // Maximum number of unsent messages. 0 to disable.
    u32 limit;

    // Current number of unsent messages.
    u32 current;

    // The number of messages dropped due to shaping.
    u32 drops;
} msg_shaper[VTSS_MODULE_ID_NONE + 1];

// This is the default limit for the shaper.
#define MSG_SHAPER_DEFAULT_LIMIT 50

#define MSG_TRACE_ENABLED(isid, modid) (((isid) == 0 || (isid) > VTSS_ISID_CNT || msg_trace_enabled_per_isid[(isid)]) && ((u32)(modid) >= VTSS_MODULE_ID_NONE || msg_trace_enabled_per_modid[(modid)]))

/****************************************************************************/
/*                                                                          */
/*  NAMING CONVENTION FOR INTERNAL FUNCTIONS:                               */
/*    RX_<function_name> : Functions related to Rx.                         */
/*    TX_<function_name> : Functions related to Tx.                         */
/*    CX_<function_name> : Functions related to both Rx and Tx (common).    */
/*    IM_<function_name> : Functions related to InitModules calls.          */
/*    DBG_<function_name>: Functions related to CLI/debugging.              */
/*                                                                          */
/*  NAMING CONVENTION FOR EXTERNAL (API) FUNCTIONS:                         */
/*    msg_rx_<function_name>: Functions related to Rx.                      */
/*    msg_tx_<function_name>: Functions related to Tx.                      */
/*    msg_<function_name>   : Functions related to both Rx and Tx.          */
/*                                                                          */
/****************************************************************************/

/******************************************************************************/
// MSG semaphores
/******************************************************************************/

// Message semaphore (must be semaphore, since it's created locked in
// one thread, and released in another).
static critd_t crit_msg_state;     // Used to protect global state and mcbs
static critd_t crit_msg_counters;  // Used to protect counters
static critd_t crit_msg_cfg;       // Used to protect subscription list
static critd_t crit_msg_pend_list; // Used to protect pending Rx and Tx done lists.
static critd_t crit_msg_im_fifo;   // Used to protect the IM_fifo.
static critd_t crit_msg_buf;       // Used to protect message allocations.

// Macros for accessing mutex functions
// ------------------------------------
#define MSG_STATE_CRIT_ENTER()             critd_enter(        &crit_msg_state,     __FILE__, __LINE__)
#define MSG_STATE_CRIT_EXIT()              critd_exit(         &crit_msg_state,     __FILE__, __LINE__)
#define MSG_STATE_CRIT_ASSERT_LOCKED()     critd_assert_locked(&crit_msg_state,     __FILE__, __LINE__)
#define MSG_COUNTERS_CRIT_ENTER()          critd_enter(        &crit_msg_counters,  __FILE__, __LINE__)
#define MSG_COUNTERS_CRIT_EXIT()           critd_exit(         &crit_msg_counters,  __FILE__, __LINE__)
#define MSG_COUNTERS_CRIT_ASSERT_LOCKED()  critd_assert_locked(&crit_msg_counters,  __FILE__, __LINE__)
#define MSG_CFG_CRIT_ENTER()               critd_enter(        &crit_msg_cfg,       __FILE__, __LINE__)
#define MSG_CFG_CRIT_EXIT()                critd_exit(         &crit_msg_cfg,       __FILE__, __LINE__)
#define MSG_CFG_CRIT_ASSERT(locked)        critd_assert_locked(&crit_msg_cfg,       __FILE__, __LINE__)
#define MSG_PEND_LIST_CRIT_ENTER()         critd_enter(        &crit_msg_pend_list, __FILE__, __LINE__)
#define MSG_PEND_LIST_CRIT_EXIT()          critd_exit(         &crit_msg_pend_list, __FILE__, __LINE__)
#define MSG_PEND_LIST_CRIT_ASSERT_LOCKED() critd_assert_locked(&crit_msg_pend_list, __FILE__, __LINE__)
#define MSG_IM_FIFO_CRIT_ENTER()           critd_enter(        &crit_msg_im_fifo,   __FILE__, __LINE__)
#define MSG_IM_FIFO_CRIT_EXIT()            critd_exit(         &crit_msg_im_fifo,   __FILE__, __LINE__)
#define MSG_IM_FIFO_CRIT_ASSERT_LOCKED()   critd_assert_locked(&crit_msg_im_fifo,   __FILE__, __LINE__)
#define MSG_BUF_CRIT_ENTER()               critd_enter(        &crit_msg_buf,       __FILE__, __LINE__)
#define MSG_BUF_CRIT_EXIT()                critd_exit(         &crit_msg_buf,       __FILE__, __LINE__)
#define MSG_BUF_CRIT_ASSERT_LOCKED()       critd_assert_locked(&crit_msg_buf,       __FILE__, __LINE__)

// Statically allocate MCBs. The master can open VTSS_ISID_CNT*MSG_CFG_CONN_CNT
// connections towards the slaves (where master itself is also considered a
// slave), whereas a slave can have MSG_CFG_CONN_CNT connections towards the
// master.
// If we are slave, only the MSG_SLV_ISID_IDX index is used.
// If we are master, the remaining VTSS_ISID_CNT entries are possibly used,
// because the master also has loopback connections towards itself.
// When using an ISID in an API call (when master), they are numbered in the
// interval [VTSS_ISID_START; VTSS_ISID_END[. This translates to an index into
// the following array of [1; VTSS_ISID_END-VTSS_ISID_START+1].
#define MSG_SLV_ISID_IDX       0
#define MSG_MST_ISID_START_IDX 1
#define MSG_MST_ISID_END_IDX   (MSG_MST_ISID_START_IDX + VTSS_ISID_CNT - 1) /* End index included */
msg_mcb_t mcbs[VTSS_ISID_CNT + 1][MSG_CFG_CONN_CNT];

// Statically allocate a global state object.
static msg_glbl_state_t state;

// TX Message Thread variables
static vtss_handle_t TX_msg_thread_handle;
static vtss_thread_t TX_msg_thread_state;  // Contains space for the scheduler to hold the current thread state.

// RX Message Thread variables
static vtss_handle_t RX_msg_thread_handle;
static vtss_thread_t RX_msg_thread_state;  // Contains space for the scheduler to hold the current thread state.

// Init Modules Thread variables.
// This thread's sole use is to call init_modules() to overcome deadlock arising if init_modules()
// were called directly from the thread, where a topology change was discovered. For instance, a call
// to init_modules() would be performed from the Packet RX thread when master and a connection was
// negotiated with a slave. This call could potentially cause the Packet RX Thread to die, because some
// modules would attempt to send messages as a result of e.g. an init_modules(Add Switch) command,
// and wait for a semaphore that wouldn't get set before the module's msg_tx_done() callback was invoked.
// This could never be invoked for as long as the Packet RX Thread was waiting for the semaphore - and
// we would have a deadlock. In fact, I think it's bad code to block the calling thread like that (the
// port module currently does), but to initially overcome these problems, we add this thread here.
static vtss_handle_t IM_thread_handle;
static vtss_thread_t IM_thread_state;
static vtss_flag_t  IM_flag; // Used to wake up the IM_thread

// For each ISID the FIFO can hold three entries: One Add, one Del, and one Default Conf.
// In addition, it can hold a master down or master up event.
#define MSG_IM_FIFO_SZ (3 * (VTSS_ISID_CNT) + 2)
#define MSG_IM_FIFO_NEXT(idx) ((idx) < ((MSG_IM_FIFO_SZ) - 1) ? (idx) + 1 : 0)
vtss_init_data_t IM_fifo[MSG_IM_FIFO_SZ];
int IM_fifo_cnt;
int IM_fifo_rd_idx;
int IM_fifo_wr_idx;

// Event flag that is used to wake up the TX_thread.
static vtss_flag_t TX_msg_flag;

// Event flag that is used to wake up the RX_thread.
static vtss_flag_t RX_msg_flag;

static msg_rx_filter_item_t *RX_filter_list = NULL;

// When calling back the User Module's Rx or Tx callback, we must not own
// the msg_state mutex. If we did, the User Module callback would not be able
// to call msg_tx() from within the callback function without a deadlock.
// Therefore, all calls to these callback functions are deferred until the
// internal state is completely updated, so that e.g. topo events can occur
// again without affecting the callback mechanism.
// The following two lists hold pending received and transmitted messages.
// The received messages are sent through the Rx dispatch mechanism, whereas
// the pending transmitted messages are sent back to the corresponding Tx Done
// user-callback functions.
// A simple mutex, crit_msg_pend_list, controls the insertion and deletion from
// the lists ready to be released. This mutex must not be held while the actual
// callback function is called, but only while inserting or removing a message
// from the list.
static msg_item_t *pend_rx_list, *pend_rx_list_last;
static msg_item_t *pend_tx_done_list, *pend_tx_done_list_last;

// Debug info
static vtss_module_id_t  dbg_latest_rx_modid;
static u32               dbg_latest_rx_len;
static u32               dbg_latest_rx_connid;
static vtss_tick_count_t dbg_max_rx_callback_ticks[VTSS_MODULE_ID_NONE + 1];

// Cached version of this switch's MAC address
static mesa_mac_addr_t this_mac;

// The following macro returns TRUE if tick counter wraps around, or if diff in now_ticks and last_ticks
// exceeds the timeout_ms. FALSE otherwise.
#define MSG_TIMEDOUT(last_ticks, now_ticks, timeout_ms) ((now_ticks)<(last_ticks) ? TRUE : ((now_ticks) - (last_ticks) >= VTSS_OS_MSEC2TICK(timeout_ms)))

// Compute the difference between a left-most SEQ number and a current,
// taking into account that the window may wrap around. This only works
// if SEQ is 16 bits wide.
#define SEQ_DIFF(left, cur) (((u32)cur >= (u32)left) ? ((u32)cur - (u32)left) : ((MSG_SEQ_CNT - (u32)left) + (u32)cur))

#define PDUTYPE2STR(p)  ((p) == MSG_PDU_TYPE_MSYN ? "MSYN" : (p) == MSG_PDU_TYPE_MSYNACK ? "MSYNACK" : (p) == MSG_PDU_TYPE_MD ? "MD" : (p) == MSG_PDU_TYPE_MACK ? "MACK" : (p) == MSG_PDU_TYPE_MRST ? "MRST" : "UNKNOWN")

/******************************************************************************/
//
/******************************************************************************/
struct msg_buf_pool_s;
typedef struct msg_buf_s {
    struct msg_buf_s      *next;
    struct msg_buf_pool_s *pool;
    u32                   ref_cnt;
    void                  *buf;
    /* Here, the message will get located */
} msg_buf_t;

/******************************************************************************/
//
/******************************************************************************/
typedef struct msg_buf_pool_s {
    u32                   magic;
    struct msg_buf_pool_s *next;
    vtss_module_id_t      module_id;
    u32                   buf_cnt_init;
    u32                   buf_cnt_cur;
    u32                   buf_cnt_min;
    u32                   buf_size;
    u32                   allocs;
    char                  *dscr;
    vtss_sem_t            sem;
    msg_buf_t             *free;
    msg_buf_t             *used;
} msg_buf_pool_t;

static msg_buf_pool_t *MSG_buf_pool;

// Note: Use an uintptr_t here to get over size differences
// when converting from address to scalar on 32- and 64-bit machines.
#define MSG_ALIGN64(x) (8 * (((uintptr_t)(x) + 7) / 8))

#define MSG_BUF_POOL_MAGIC 0xbadebabe

/****************************************************************************/
// msg_flash_switch_info_t
// Holds various info about a given switch in the stack. This info is saved
// to flash and loaded at master up event from topo.
// It facilitates transferring info about all previously seen switches in the
// stack to other modules (read: the port module) in master up events.
// In this way, the port module can provide the correct port count and stack
// port numbers to other modules, and the message module can tell whether
// the switch is configurable or not.
/****************************************************************************/
typedef struct {
    u32 version; // Version of this structure in the flash.

    init_switch_info_t info[VTSS_ISID_END]; // VTSS_ISID_LOCAL unused.
} msg_flash_switch_info_t;

static msg_flash_switch_info_t CX_switch_info;

// When a stacking build is configured for standalone,
// we expose CX_switch_info.info[X].configurable as FALSE for
// all switches, but the switch itself.
// However, in order to be able to preserve the switch info
// whenever we write back to flash, we need to save a copy of
// the "real" configurable flag. This is what this array is for.
static BOOL CX_switch_really_configurable[VTSS_ISID_END];

#define MSG_FLASH_SWITCH_INFO_VER 1

/****************************************************************************/
/*                                                                          */
/*  MODULE INTERNAL FUNCTIONS                                               */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
// CX_event_init()
/****************************************************************************/
static void CX_event_init(vtss_init_data_t *event)
{
    memset(event, 0, sizeof(*event));
}

/****************************************************************************/
/*                                                                          */
/*  INIT MODULES HELPER INTERNAL FUNCTIONS                                  */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
// This one holds the state reported to the user modules.
// The idea is that once a SWITCH_ADD event is passed to the user modules,
// msg_switch_exists() returns TRUE whether or not the switch goes away
// during this event.
// Likewise, once a MASTER_UP (ICFG_LOADING_PRE) event is passed to the user
// modules, msg_switch_is_primary() returns TRUE throughout the event whether or
// not the message module protocol knows that it is now a slave.
//
// The structure also holds a cached version of the master ISID.
//
// Besides caching the info from the message module protocol, it also serves
// to avoid race-conditions as follows:
//
// Suppose you have two unconnected switches. Since the switches are not connected
// to each other, they are both masters. Now, connect them and observe the
// following timeline (supposing switch #1 becomes the new master of the stack):
//
// Time Switch #1            Switch #2
// ---- -------------------- --------------------
//    0 No event             No event
//    1 No event             SWITCH_DEL(isid = 2)
//    2 SWITCH_ADD(isid = 2) No event
//    3 No event             MASTER_DOWN
//
// Here, it is assumed that the SWITCH_DEL event on the upcoming slave takes quite
// a while to pump through the user modules. So in time step 0, 1, and 2, the
// upcoming slave still believes it's master, whereas switch #1 thinks that from
// time step 2, switch #2 is slave, which means that switch #2 may discard messages
// from switch #1 in this period of time (at the user module level; at the message
// module level, switch #2 indeed knows that it is slave).
//
// This race condition is solved by holding back transmission of MSYNACKS from
// switch #2 towards switch #1 until the MASTER_DOWN event has been passed to
// all user modules. Switch #1 will retransmit MSYN packets until switch #2
// starts replying. The new timeline will therefore become:
//
// Time Switch #1            Switch #2
// ---- -------------------- --------------------
//    0 No event             No event
//    1 No event             SWITCH_DEL(isid = 2)
//    2 No event             MASTER_DOWN
//    3 No event             (start replying to MSYNs)
//    3 SWITCH_ADD(isid = 2) No event
//
// The structure is protected by MSG_STATE_CRIT_ENTER()/EXIT()
/****************************************************************************/
static struct {
    BOOL        master;
    BOOL        exists[VTSS_ISID_END];
    vtss_isid_t master_isid; // VTSS_ISID_UNKNOWN when not master or when the first SWITCH_ADD event hasn't happened.
    BOOL        ignore_msyns;
} CX_user_state;

// We can no longer generate the master down event
#define INIT_CMD_MASTER_DOWN INIT_CMD_OBSOLETE
#define INIT_CMD_SWITCH_DEL  INIT_CMD_OBSOLETE_2

/****************************************************************************/
// IM_user_state_update()
/****************************************************************************/
static void IM_user_state_update(vtss_init_data_t *event)
{
    MSG_STATE_CRIT_ENTER();

    switch (event->cmd) {
    case INIT_CMD_ICFG_LOADING_PRE:
        CX_user_state.master = TRUE;
        CX_user_state.master_isid = VTSS_ISID_UNKNOWN; // Hardly needed.

        // Let threads that should be started prior to us pumping the
        // MASTER_UP/ICFG_LOADING_PRE event around run.
        CX_master_up_pre_lock.lock(false);
        break;

    case INIT_CMD_MASTER_DOWN:
        CX_user_state.master = FALSE;
        CX_user_state.master_isid = VTSS_ISID_UNKNOWN;
        // We're no longer master. Let threads that eventually suspend
        // themselves with a call to msg_wait() remain suspended.
        CX_master_up_pre_lock.lock(true);
        CX_master_up_post_lock.lock(true);
        CX_switch_add_post_lock.lock(true);
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        CX_user_state.exists[event->isid] = TRUE;
        if (CX_user_state.master_isid == VTSS_ISID_UNKNOWN) {
            // The first added switch is the master.
            CX_user_state.master_isid = event->isid;
        }
        break;

    case INIT_CMD_SWITCH_DEL:
        CX_user_state.exists[event->isid] = FALSE;
        if (CX_user_state.master_isid == event->isid) {
            CX_user_state.master_isid = VTSS_ISID_UNKNOWN;
        }
        break;

    default:
        break;
    }

    MSG_STATE_CRIT_EXIT();
}

/****************************************************************************/
// IM_thread()
// The thread that dispatches state changes to other modules through the
// init_modules() function.
/****************************************************************************/
static void IM_thread(vtss_addrword_t thr_data)
{
    vtss_init_data_t event;




    // Wait until INIT_CMD_INIT is complete.
    msg_wait(MSG_WAIT_UNTIL_INIT_DONE, VTSS_MODULE_ID_MSG);

    // We've taken over the role of calling init_modules with INIT_CMD_START
    // from the main_thread in main.c. The reason for this is that if it
    // wasn't so, then it might happen that TOPO was running before the main
    // thread, providing the default master-up to the message module, which
    // in turn would send it to this thread, which in turn would call
    // init_modules with MASTER_UP/ICFG_LOADING_PRE - possibly before main had
    // called it with INIT_CMD_START.
    CX_event_init(&event);
    event.cmd = INIT_CMD_START;
    (void)init_modules(&event);

    while (1) {



























        (void)vtss_flag_wait(&IM_flag, 0xFFFFFFFF, VTSS_FLAG_WAITMODE_OR_CLR);


        // Empty the FIFO.
        MSG_IM_FIFO_CRIT_ENTER();

        while (IM_fifo_cnt > 0) {
            event = IM_fifo[IM_fifo_rd_idx];
            IM_fifo_rd_idx = MSG_IM_FIFO_NEXT(IM_fifo_rd_idx);
            IM_fifo_cnt--;
            MSG_IM_FIFO_CRIT_EXIT();

            IM_user_state_update(&event);

            if (MSG_TRACE_ENABLED(event.isid, -1)) {
                T_IG(TRACE_GRP_INIT_MODULES, "Enter. cmd=%s, isid=%u", control_init_cmd2str(event.cmd), event.isid);
            }

            (void)init_modules(&event);

            if (MSG_TRACE_ENABLED(event.isid, -1)) {
                T_IG(TRACE_GRP_INIT_MODULES, "Exit. cmd=%s, isid=%u", control_init_cmd2str(event.cmd), event.isid);
            }

            if (event.cmd == INIT_CMD_ICFG_LOADING_PRE) {
                // Now that all modules know that we're master let their threads run.
                CX_master_up_post_lock.lock(false);
            } else if (event.cmd == INIT_CMD_ICFG_LOADING_POST) {
                CX_switch_add_post_lock.lock(false);
            } else if (event.cmd == INIT_CMD_MASTER_DOWN) {
                // Time to no longer ignore MSYNs, since the user modules have
                // been passed the info of a master down (see thoughts about this
                // right above the CX_user_state structure).
                MSG_STATE_CRIT_ENTER();
                CX_user_state.ignore_msyns = FALSE;
                MSG_STATE_CRIT_EXIT();
                T_IG(TRACE_GRP_INIT_MODULES, "Accepting MSYNs");
            }

            // Prepare for next loop
            MSG_IM_FIFO_CRIT_ENTER();
        }

        // Done
        MSG_IM_FIFO_CRIT_EXIT();
    }
}

/******************************************************************************/
// IM_fifo_put()
/******************************************************************************/
static BOOL IM_fifo_put(vtss_init_data_t *new_event)
{
    BOOL result;

    T_IG(TRACE_GRP_INIT_MODULES, "Add(FIFO): cmd=%s, parm=%d", control_init_cmd2str(new_event->cmd), new_event->isid);
    MSG_IM_FIFO_CRIT_ENTER();

    if (IM_fifo_cnt == MSG_IM_FIFO_SZ) {
        // FIFO is full.
        vtss_init_data_t latest_data;
        const char       *latest_init_module_func_name;
        control_dbg_latest_init_modules_get(&latest_data, &latest_init_module_func_name);
        T_EG(TRACE_GRP_INIT_MODULES, "IM_fifo is full (cmd=%s, isid=%d). Latest init_modules props: cmd=%s, isid=%u flags=0x%x, init-func=%s", control_init_cmd2str(new_event->cmd), new_event->isid, control_init_cmd2str(latest_data.cmd), latest_data.isid, latest_data.flags, latest_init_module_func_name);
        result = FALSE;
    } else {
        IM_fifo[IM_fifo_wr_idx] = *new_event;
        IM_fifo_wr_idx = MSG_IM_FIFO_NEXT(IM_fifo_wr_idx);
        IM_fifo_cnt++;
        result = TRUE;
    }

    // Wake up the IM_thread()
    vtss_flag_setbits(&IM_flag, 1);
    MSG_IM_FIFO_CRIT_EXIT();
    return result;
}

/****************************************************************************/
/*                                                                          */
/*  COMMON INTERNAL FUNCTIONS, PART 1                                       */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
// TX_wake_up_msg_thread()
/****************************************************************************/
static void TX_wake_up_msg_thread(int flag)
{
    // Wake-up the TX_thread().
    vtss_flag_setbits(&TX_msg_flag, flag);
}

/****************************************************************************/
// RX_wake_up_msg_thread()
/****************************************************************************/
static void RX_wake_up_msg_thread(int flag)
{
    // Wake-up the RX_thread().
    vtss_flag_setbits(&RX_msg_flag, flag);
}

/****************************************************************************/
// CX_uptime_secs_get()
// Returns the number of seconds that has elapsed since boot.
/****************************************************************************/
static u32 CX_uptime_secs_get(void)
{
    return vtss::uptime_seconds();
}

/****************************************************************************/
// CX_uptime_get_crit_taken()
// Returns the up-time measured in seconds of a switch.
// The MSG_STATE_CRIT_ENTER() must have been taken prior to this call.
// See msg_uptime_get() for a thorough description.
/****************************************************************************/
static u32 CX_uptime_get_crit_taken(vtss_isid_t isid)
{
    u32 cur_uptime;

    VTSS_ASSERT(VTSS_ISID_LEGAL(isid) || isid == VTSS_ISID_LOCAL);

    if (isid == VTSS_ISID_LOCAL) {
        // Just use the current time.
        return CX_uptime_secs_get();
    } else if (state.state == MSG_MOD_STATE_PRI && mcbs[isid][0].state == MSG_CONN_STATE_PRI_ESTABLISHED) {
        // We're master and the connection is established.
        cur_uptime = CX_uptime_secs_get();
        if (cur_uptime < mcbs[isid][0].u.primary_switch.switch_info.mst_uptime_secs) {
            // This may occur at roll over (after 136 years).
            T_W("Current uptime (%u) is smaller than that of connection establishment (%u)", cur_uptime, mcbs[isid][0].u.primary_switch.switch_info.mst_uptime_secs);
            cur_uptime = mcbs[isid][0].u.primary_switch.switch_info.mst_uptime_secs;
        }

        return (mcbs[isid][0].u.primary_switch.switch_info.slv_uptime_secs + (cur_uptime - mcbs[isid][0].u.primary_switch.switch_info.mst_uptime_secs));
    }

    // We're not primary switch or the slave is not connected.
    return 0;
}

/******************************************************************************/
// CX_concat_msg_items()
// Concatenates the list pointed to by msg_first to list_last, and updates
// list (if needed) and list_last.
// msg_first may be NULL, i.e. this function can be called even when there are
// no messages to concatenate to an existing or empty list.
/******************************************************************************/
static void CX_concat_msg_items(msg_item_t **list, msg_item_t **list_last, msg_item_t *msg_first, msg_item_t *msg_last)
{
    if (!msg_first) {
        return; // Nothing to concatenate.
    }

    VTSS_ASSERT(msg_last->next == NULL);

    // Link it in
    if (*list_last == NULL) {
        // No items currently in list.
        VTSS_ASSERT(*list == NULL);
        *list      = msg_first;
    } else {
        // Already items in the list. Append to it.
        VTSS_ASSERT(*list && (*list_last)->next == NULL);
        (*list_last)->next = msg_first;
    }

    *list_last = msg_last;
}

/****************************************************************************/
// CX_get_pend_list()
/****************************************************************************/
static msg_item_t *CX_get_pend_list(msg_item_t **pend_list, msg_item_t **pend_list_last)
{
    msg_item_t *result;
    // Get list mutex before messing with the list.
    MSG_PEND_LIST_CRIT_ENTER();
    result = *pend_list;

    if (*pend_list) {
        *pend_list = (*pend_list)->next;
    }

    if (*pend_list == NULL) {
        *pend_list_last = NULL;
    }

    MSG_PEND_LIST_CRIT_EXIT();
    return result;
}

/****************************************************************************/
// CX_put_pend_list()
/****************************************************************************/
static void CX_put_pend_list(msg_item_t **pend_list, msg_item_t **pend_list_last, msg_item_t *msg_list, msg_item_t *msg_list_last)
{
    // Get list mutex before messing with the list.
    MSG_PEND_LIST_CRIT_ENTER();
    CX_concat_msg_items(pend_list, pend_list_last, msg_list, msg_list_last);
    MSG_PEND_LIST_CRIT_EXIT();
}

/******************************************************************************/
// CX_switch_info_valid()
/******************************************************************************/
static BOOL CX_switch_info_valid(init_switch_info_t *info)
{
    if (info->configurable) {
        if (info->port_cnt > fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
            T_W("Invalid port count: %u", info->port_cnt);
            return FALSE;
        }

        if (info->stack_ports[0] == VTSS_PORT_NO_NONE) {
            // Not stackable. Only possible if the info is ourselves.
            if (info->stack_ports[1] != VTSS_PORT_NO_NONE) {
                T_E("Invalid stack port(1)");
                return FALSE;
            }
        } else if (info->stack_ports[0] >= info->port_cnt ||
                   info->stack_ports[1] >= info->port_cnt ||
                   info->stack_ports[0] == info->stack_ports[1]) {
            T_E("Invalid stack port(s)");
            return FALSE;
        }
    }

    return TRUE;
}

/****************************************************************************/
// CX_init_switch_info_get()
/****************************************************************************/
static void CX_init_switch_info_get(init_switch_info_t *switch_info)
{
    switch_info->configurable   = TRUE;
    switch_info->port_cnt       = port_count_max();
    switch_info->stack_ports[0] = VTSS_PORT_NO_NONE;
    switch_info->stack_ports[1] = VTSS_PORT_NO_NONE;
    switch_info->board_type     = vtss_board_type();
    switch_info->api_inst_id    = misc_chiptype();

    // Check that what we got from the port module indeed is valid.
    // The function prints the necessary errors.
    (void)CX_switch_info_valid(switch_info);
}

/****************************************************************************/
// CX_local_switch_info_get()
/****************************************************************************/
static void CX_local_switch_info_get(msg_switch_info_t *switch_info)
{
    u32 uptime;

    memset(switch_info, 0, sizeof(*switch_info));

    // Local version string
    strncpy(switch_info->version_string, misc_software_version_txt(), MSG_MAX_VERSION_STRING_LEN); // From version.h
    switch_info->version_string[MSG_MAX_VERSION_STRING_LEN - 1] = '\0';

    // Local product name
    strncpy(switch_info->product_name, misc_product_name(), MSG_MAX_PRODUCT_NAME_LEN); // From version.h
    switch_info->product_name[MSG_MAX_PRODUCT_NAME_LEN - 1] = '\0';

    // Local uptime
    uptime = CX_uptime_secs_get();
    switch_info->slv_uptime_secs = uptime;
    switch_info->mst_uptime_secs = uptime; // May not be used.

    // Other info
    CX_init_switch_info_get(&switch_info->info);
}

/******************************************************************************/
// CX_flash_do_write()
/******************************************************************************/
static void CX_flash_do_write(msg_flash_switch_info_t *flash_info)
{
    vtss_isid_t isid;

    if (flash_info) {
        *flash_info = CX_switch_info;

        // Copy the shadow configurable flags back into the flash_info structure.
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            flash_info->info[isid].configurable = CX_switch_really_configurable[isid];
        }

        flash_info->version = MSG_FLASH_SWITCH_INFO_VER;
        conf_sec_close(CONF_SEC_GLOBAL, CONF_BLK_MSG);
    }
}

/******************************************************************************/
// CX_flash_write()
/******************************************************************************/
static void CX_flash_write(void)
{
    msg_flash_switch_info_t *flash_info;
    ulong                   size;

    MSG_STATE_CRIT_ASSERT_LOCKED();

    if ((flash_info = (msg_flash_switch_info_t *)conf_sec_open(CONF_SEC_GLOBAL, CONF_BLK_MSG, &size)) == NULL || size != sizeof(*flash_info)) {
        T_W("Failed to open flash configuration");
        return;
    }

    CX_flash_do_write(flash_info);
}

/******************************************************************************/
// CX_flash_read()
// master_isid is only used if creating defaults, and only if it is a legal isid.
// If it's an illegal ISID, CX_switch_info is not overwritten if defaults
// are created.
/******************************************************************************/
static void CX_flash_read(vtss_isid_t master_isid)
{
    msg_flash_switch_info_t *flash_info;
    ulong                   size;
    vtss_isid_t             isid;
    BOOL                    create_defaults = FALSE;

    if ((flash_info = (msg_flash_switch_info_t *)conf_sec_open(CONF_SEC_GLOBAL, CONF_BLK_MSG, &size)) == NULL || size != sizeof(*flash_info)) {
        T_W("conf_sec_open() failed or size mismatch. Creating defaults.");
        flash_info = (msg_flash_switch_info_t *)conf_sec_create(CONF_SEC_GLOBAL, CONF_BLK_MSG, sizeof(*flash_info));
        create_defaults = TRUE;
    } else if (flash_info->version != MSG_FLASH_SWITCH_INFO_VER) {
        T_W("Version mismatch. Creating defaults.");
        create_defaults = TRUE;
    }

    if (!create_defaults && flash_info) {
        // Integrity check.
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            if (!CX_switch_info_valid(&flash_info->info[isid])) {
                create_defaults = TRUE;
                break;
            }
        }
    }

    MSG_STATE_CRIT_ASSERT_LOCKED();

    if (create_defaults) {
        if (VTSS_ISID_LEGAL(master_isid)) {
            // Prevent this function from updating CX_switch_info if
            // called with an illegal isid (since that's for debugging only).
            memset(&CX_switch_info, 0, sizeof(CX_switch_info));
        }
    } else if (flash_info) {
        CX_switch_info = *flash_info;
    }

    // Always update the local switch info section, provided we're called
    // with a valid ISID. If we didn't update it, it could happen that two
    // different builds running on the same board would cause problems
    // when loading the second build, when properties from the first build
    // were successfully loaded from flash.
    if (VTSS_ISID_LEGAL(master_isid)) {
        CX_init_switch_info_get(&CX_switch_info.info[master_isid]);

#if defined(VTSS_SW_OPTION_MSG_HOMOGENEOUS_STACK)
        // In a homogeneous stack, all switches are identical (i.e.
        // you don't have support for both 24- and 48-ported at the same
        // time, and all PHYs are of the same type (either Cu or SFP)).
        // In such cases, we can make all switches configurable right
        // away, so that a switch can be configured before it has ever
        // been seen in a stack. We assume that the unconfigurable ISIDs
        // have exactly the same layout as this master switch (stack ports as well).
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            if (isid != master_isid) {
                init_switch_info_t *si = &CX_switch_info.info[isid];
                if (!si->configurable) {
                    *si = CX_switch_info.info[master_isid];
                }
            }
        }
#endif
    }

    // If stacking is disabled, we copy all configurable flags from
    // the "public" structure into a local-only one, and clear
    // the "public" configurable flag for all-but-the-master switch
    // (this local switch). This ensures that we can say "no" to
    // msg_switch_configurable() when invoked with an ISID that is not
    // ourselves in standalone mode. When (if) configuration is saved back
    // into flash, these flags are restored in the flash copy.
    if (VTSS_ISID_LEGAL(master_isid)) {
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            CX_switch_really_configurable[isid] = CX_switch_info.info[isid].configurable;
            // if statement cannot become true. Removed to pass coverity check
            // if (isid != master_isid) {
            //     // Pretend that slaves are not configurable in standalone mode
            //     CX_switch_info.info[isid].configurable = FALSE;
            // }
        }
    }

    CX_flash_do_write(flash_info);
}

/****************************************************************************/
// CX_event_conf_def()
/****************************************************************************/
static BOOL CX_event_conf_def(vtss_isid_t isid, init_switch_info_t *new_info)
{
    vtss_init_data_t event;

    CX_event_init(&event);
    event.cmd  = INIT_CMD_CONF_DEF;
    event.isid = isid;
    event.switch_info[isid] = *new_info;
    return IM_fifo_put(&event);
}

/******************************************************************************/
// CX_switch_info_update()
/******************************************************************************/
static BOOL CX_switch_info_update(vtss_isid_t isid, init_switch_info_t *new_info)
{
    init_switch_info_t *cur_info = &CX_switch_info.info[isid];

    MSG_STATE_CRIT_ASSERT_LOCKED();

#if !defined(VTSS_SW_OPTION_MSG_HOMOGENEOUS_STACK)
    // For homogeneous stacks, simply overwrite, but don't
    // reset configuration to defaults.
    // For heterogenous stacks we may need to reset configuration
    // because of a different port count or board type.
    if (CX_switch_really_configurable[isid]) {
        // The switch has already been seen previously.
        // Check to see if the new switch has the same configuration
        // as the previous. Check all fields, except for stack ports.
        if (new_info->port_cnt    != cur_info->port_cnt   ||
            new_info->board_type  != cur_info->board_type ||
            new_info->api_inst_id != cur_info->api_inst_id) {
            // There's differences. Gotta generate a conf-def event.
            if (!CX_event_conf_def(isid, new_info)) {
                // Don't even attempt to save the configuration
                // if the FIFO put operation failed, because we
                // really need to generate a conf-def event, and therefore
                // must re-negotiate the connection and try again the
                // next time it's negotiated. If we saved to flash
                // here, there would be no differences the next time
                // we get here, and the conf-def event would miss out.
                return FALSE;
            }
        }
    }
#endif

    // We save to flash whenever the info has changed.
    CX_switch_really_configurable[isid] = new_info->configurable;
    if (memcmp(new_info, cur_info, sizeof(*new_info)) != 0) {
        *cur_info = *new_info;
        CX_flash_write();
    }

    return TRUE;
}

/****************************************************************************/
// CX_event_switch_add()
/****************************************************************************/
static BOOL CX_event_switch_add(vtss_isid_t isid, init_switch_info_t *switch_info)
{
    vtss_init_data_t event;

    // First update our flash configuration. This may imply distributing
    // a conf-def event.
    if (!CX_switch_info_update(isid, switch_info)) {
        return FALSE;
    }

    CX_event_init(&event);
    event.cmd  = INIT_CMD_ICFG_LOADING_POST;
    event.isid = isid;
    event.switch_info[isid] = *switch_info;
    (void)IM_fifo_put(&event);

    CX_event_init(&event);
    event.cmd = INIT_CMD_ICFG_LOADED_POST;
    event.isid = isid;
    event.switch_info[isid] = *switch_info;
    return IM_fifo_put(&event);
}

/****************************************************************************/
// RX_put_list()
/****************************************************************************/
static void RX_put_list(msg_mcb_t *mcb, msg_item_t *msg_list, msg_item_t *msg_list_last)
{
    msg_item_t *msg = msg_list;

    while (msg) {
        mcb->stat.rx_msg++; // Gotta increase the counter here, because when the msg has been moved, we lose the information of origin.
        if (msg->is_tx_msg) {
            // Count loopbacks in the Tx list as well.
            mcb->stat.tx_msg[0]++;
        }
        msg = msg->next;
    }

    CX_put_pend_list(&pend_rx_list, &pend_rx_list_last, msg_list, msg_list_last);

    // Wake up the RX_thread(). This may cause a "spurious" wake-up, since
    // RX_put_list() may be called from the TX_thread() in the loop-back case,
    // but this doesn't matter.
    RX_wake_up_msg_thread(MSG_FLAG_RX_MSG);
}

/****************************************************************************/
// TX_put_done_list()
/****************************************************************************/
static void TX_put_done_list(msg_mcb_t *mcb, msg_item_t *msg_list, msg_item_t *msg_list_last)
{
    msg_item_t *msg = msg_list;

    if (mcb) {
        while (msg) {
            mcb->stat.tx_msg[msg->u.tx.rc == MSG_TX_RC_OK ? 0 : 1]++;
            msg = msg->next;
        }
    }

    CX_put_pend_list(&pend_tx_done_list, &pend_tx_done_list_last, msg_list, msg_list_last);

    TX_wake_up_msg_thread(MSG_FLAG_TX_DONE_MSG);
}

/****************************************************************************/
// TX_put_done_list_front()
// Used to un-get a tx done message if it wasn't acknowledged Tx'd by the FDMA.
/****************************************************************************/
static MSG_INLINE void TX_put_done_list_front(msg_item_t *msg)
{
    if (!msg) {
        return;
    }

    MSG_PEND_LIST_CRIT_ENTER();
    msg->next = pend_tx_done_list;
    if (pend_tx_done_list_last == NULL) {
        pend_tx_done_list_last = msg;
    }

    pend_tx_done_list = msg;
    MSG_PEND_LIST_CRIT_EXIT();
}

/******************************************************************************/
// CX_mcb_flush()
// Moves the mcb's tx_msg_list to the global pend_tx_done_list while assigning it
// an error number. Doing so causes the TX_thread() to callback any user-
// defined Tx done handlers outside the holding of the msg_state_crit
// semaphore.
// The function also releases any received MDs.
// May be called in both slave and master mode.
/******************************************************************************/
static void CX_mcb_flush(msg_mcb_t *mcb, msg_tx_rc_t reason)
{
    msg_item_t *tx_msg, *rx_msg;

    VTSS_ASSERT(reason != MSG_TX_RC_OK);

    MSG_STATE_CRIT_ASSERT_LOCKED();

    // Move the TX message list to the Tx Done list, which also serves
    // as holding messages not Tx'd.
    if (mcb->tx_msg_list) {
        tx_msg = mcb->tx_msg_list;

        // Walk through the list of unsent/unacknowledged user messages and
        // assign the rc member the value in reason, so that the user function
        // can get called back with the right explanation.
        while (tx_msg) {
            tx_msg->u.tx.rc = reason;
            tx_msg = tx_msg->next;
        }

        TX_put_done_list(mcb, mcb->tx_msg_list, mcb->tx_msg_list_last);

        // No more pending messages for this connection.
        mcb->tx_msg_list      = NULL;
        mcb->tx_msg_list_last = NULL;
    }

    // Also flush the RX message list.
    rx_msg = mcb->rx_msg_list;
    while (rx_msg) {
        msg_item_t *rx_msg_next = rx_msg->next;
        VTSS_FREE(rx_msg->usr_msg);
        VTSS_FREE(rx_msg);
        rx_msg = rx_msg_next;
    }

    mcb->rx_msg_list = NULL;
}

/******************************************************************************/
// CX_set_state_mst_no_slv()
/******************************************************************************/
static void CX_set_state_mst_no_slv(msg_mcb_t *mcb)
{
    mcb->state = MSG_CONN_STATE_PRI_NO_SEC;
    // Clear MAC address so that we don't get false hits when receiving MSYNACKs or MDs.
    memset(mcb->dmac, 0, sizeof(mesa_mac_addr_t));
}

/******************************************************************************/
// CX_restart()
/******************************************************************************/
static void CX_restart(msg_mcb_t *mcb, vtss_isid_t isid, u8 cid, msg_tx_rc_t slv_reason, msg_tx_rc_t mst_reason)
{
    if (state.state == MSG_MOD_STATE_SEC) {
        // If we're slave, we transmit an MRST to the master and go to the "No Master" state.
        // Eventually the master will re-negotiate the connection.
        T_I("Restarting. Reason: %d", slv_reason);

        CX_mcb_flush(mcb, slv_reason);
        mcb->state = MSG_CONN_STATE_SEC_NO_PRI;
    } else {
        vtss_init_data_t event;

        T_I("Restarting. Reason: %d", mst_reason);

        // In master state, we go to the "start Txing MSYNs" to re-negotiate connection.
        CX_mcb_flush(mcb, mst_reason);

        // Clear the DMAC, so that we don't get false hits when receiving MSYNACKs or MDs.
        memset(mcb->dmac, 0, sizeof(mesa_mac_addr_t));
        mcb->state = MSG_CONN_STATE_PRI_RDY;

        // Notify user modules of the event
        CX_event_init(&event);
        event.cmd = INIT_CMD_SWITCH_DEL;
        event.isid = isid;
        (void)IM_fifo_put(&event);
    }
}

/******************************************************************************/
// CX_state2str()
/******************************************************************************/
static const char *CX_state2str(msg_conn_state_t astate)
{
    switch (astate) {
    case MSG_CONN_STATE_SEC_NO_PRI:
        return "No Master";

    case MSG_CONN_STATE_SEC_WAIT_FOR_MSYNACKS_MACK:
        return "W(MACK)";

    case MSG_CONN_STATE_SEC_ESTABLISHED:
        return "Connected";

    case MSG_CONN_STATE_PRI_NO_SEC:
        return "No Slave";

    case MSG_CONN_STATE_PRI_RDY:
        return "Connecting";

    case MSG_CONN_STATE_PRI_WAIT_FOR_MSYNACK:
        return "W(MSYNACK)";

    case MSG_CONN_STATE_PRI_ESTABLISHED:
        return "Connected";

    case MSG_CONN_STATE_PRI_STOP:
        return "CID N/A";

    default:
        return "Unknown";
    }
}

/******************************************************************************/
// CX_free_msg()
// Frees the MDs pointed to by @msg, possibly calls back the user-defined
// callback function, possibly frees the user-message, and finally frees
// the msg structure itself.
// Therefore, after this call returns, @msg is no longer valid.
//
// NOTE 1:
// The @msg must be detached from the state and mcbs arrays before this function
// is called, so that other threads don't think it's still there, because it
// won't be after this call.
//
// NOTE 2: This function must not be called while owning the crit_msg_state.
// Failing to observe this rule could cause a deadlock if the user's callback
// function calls msg_tx() or other API functions that attempt to aquire the
// mutex.
/******************************************************************************/
static void CX_free_msg(msg_item_t *msg)
{
    if (msg->is_tx_msg) {
        vtss_module_id_t dmodid = MIN(msg->dmodid, VTSS_MODULE_ID_NONE);

        // Count the event in the msg->rc bin for the current module state.
        // state.state holds the current state, which is used as an index
        // into the state.state_stat(istics) array's tx member, which is an array
        // that contains an entry for every reason for the call to to this
        // function, i.e. whether it was Tx'd OK or not.
        MSG_COUNTERS_CRIT_ENTER();
        VTSS_ASSERT(msg->u.tx.rc < MSG_TX_RC_LAST_ENTRY);
        state.usr_stat[msg->state].tx_per_return_code[msg->u.tx.rc]++;
        state.usr_stat[msg->state].tx[dmodid][msg->u.tx.rc == MSG_TX_RC_OK ? 0 : 1]++;
        state.usr_stat[msg->state].txb[dmodid] += msg->len;
        MSG_COUNTERS_CRIT_EXIT();

        // Get mutex
        MSG_STATE_CRIT_ENTER();

        // Also decrement the current outstanding count if shaping the message
        if (msg->u.tx.opt & MSG_TX_OPT_SHAPE) {
            VTSS_ASSERT(msg_shaper[dmodid].current > 0);
            msg_shaper[dmodid].current--;
        }

        MSG_STATE_CRIT_EXIT();

        if (MSG_TRACE_ENABLED(msg->connid, dmodid)) {
            T_IG(TRACE_GRP_TX, "TxDone(Msg): (len=%u, did=%u, dmodid=%s, %s, rc=%u)", msg->len, msg->connid, vtss_module_names[dmodid], (msg->u.tx.opt & MSG_TX_OPT_DONT_FREE) ? "Owner frees" : "Msg Module frees", msg->u.tx.rc);
            T_NG(TRACE_GRP_TX, "len=%u (first %u bytes shown)", msg->len, MIN(96, msg->len));
            T_NG_HEX(TRACE_GRP_TX, msg->usr_msg, MIN(96, msg->len));
        }

        // Callback User Module if it wants the result.
        if (msg->u.tx.cb) {
            if (MSG_TRACE_ENABLED(msg->connid, dmodid)) {
                T_DG(TRACE_GRP_TX, "TxDone(Msg): Callback enter (len = %u, dmodid = %s)", msg->len, vtss_module_names[dmodid]);
            }

            msg->u.tx.cb(msg->u.tx.contxt, msg->usr_msg, msg->u.tx.rc);
            if (MSG_TRACE_ENABLED(msg->connid, msg->dmodid)) {
                T_DG(TRACE_GRP_TX, "TxDone(Msg): Callback exit (len = %u, dmodid = %s)", msg->len, vtss_module_names[dmodid]);
            }
        }

        // Free the memory used by the User Message if asked to.
        if ((msg->u.tx.opt & MSG_TX_OPT_DONT_FREE) == 0) {
            if (MSG_TRACE_ENABLED(msg->connid, dmodid)) {
                T_DG(TRACE_GRP_TX, "TxDone(Msg): Freeing user message");
            }
            VTSS_FREE(msg->usr_msg);
        }
    } else {
        // msg is an Rx message.
        // Free the user message, since it's dynamically allocated.
        VTSS_FREE(msg->usr_msg);
    }

    // Also free the msg structure itself
    VTSS_FREE(msg);
}

/****************************************************************************/
/*                                                                          */
/*  TX INTERNAL FUNCTIONS                                                   */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/*  RX INTERNAL FUNCTIONS                                                   */
/*                                                                          */
/****************************************************************************/

/******************************************************************************/
// RX_filter_validate()
/******************************************************************************/
static BOOL RX_filter_validate(const msg_rx_filter_t *filter)
{
    msg_rx_filter_item_t *l = RX_filter_list;

    if (!filter->cb) {
        T_EG(TRACE_GRP_CFG, "Callback function not defined");
        return FALSE;
    }

    // Currently only support one filter. Check to see if other modules have
    // registered for it.
    while (l) {
        if (l->filter.modid == filter->modid) {
            T_EG(TRACE_GRP_CFG, "Another module has already registered for this module ID (%d)", filter->modid);
            return FALSE;
        }

        l = l->next;
    }

    return TRUE;
}

/******************************************************************************/
// RX_filter_insert()
/******************************************************************************/
static BOOL RX_filter_insert(const msg_rx_filter_t *filter)
{
    msg_rx_filter_item_t *l = RX_filter_list, *new_l;

    // Critical region must be obtained when this function is called.
    MSG_CFG_CRIT_ASSERT(1);

    // Allocate a new list item
    if ((VTSS_MALLOC_CAST(new_l, sizeof(msg_rx_filter_item_t))) == NULL) {
        T_EG(TRACE_GRP_CFG, "VTSS_MALLOC(msg_rx_filter_item_t) failed");
        return FALSE;
    }

    // Copy the filter
    memcpy(&new_l->filter, filter, sizeof(new_l->filter));

    // Insert the filter in the list. Since there's no inherited priority,
    // we simply insert it in the beginning of the list, because that's easier.
    RX_filter_list = new_l;
    new_l->next = l;

    return TRUE;
}

/******************************************************************************/
// RX_msg_dispatch()
// Pass the message to the registered handler and count the event.
// Deallocation of @msg is not handled by this function!
//
// NOTE 1: The caller of this function must not be the owner of the crit_msg_state!
//
// NOTE 2: This function may be called from different threads. Updating of the
// counters are protected.
/******************************************************************************/
static MSG_INLINE void RX_msg_dispatch(msg_item_t *msg)
{
    BOOL                 handled = FALSE;
    msg_rx_filter_item_t *l;
    u8                   dmodid = msg->dmodid;

    if (MSG_TRACE_ENABLED(msg->connid, dmodid)) {
        if (msg->is_tx_msg) {
            // It's a loopback message
            T_IG(TRACE_GRP_RX, "Rx(Msg): len=%u, mod=%s, connid=%u", msg->len, vtss_module_names[dmodid], msg->connid);
        } else {
            // It's really received on a port
            T_IG(TRACE_GRP_RX, "Rx(Msg): len=%u, mod=%s, connid=%u, seq=[%u,%u]", msg->len, vtss_module_names[dmodid], msg->connid, msg->u.rx.left_seq, msg->u.rx.right_seq);
        }

        T_NG(TRACE_GRP_RX, "len=%u (first %u bytes shown)", msg->len, MIN(96, msg->len));
        T_NG_HEX(TRACE_GRP_RX, msg->usr_msg, MIN(96, msg->len));
    }

    // Protect the filter list.
    MSG_CFG_CRIT_ENTER();
    l = RX_filter_list;
    while (l) {
        if (l->filter.modid == dmodid) {
            vtss_tick_count_t start, total;

            // Save some debugging info.
            dbg_latest_rx_modid  = dmodid;
            dbg_latest_rx_len    = msg->len;
            dbg_latest_rx_connid = msg->connid;
            start = vtss_current_time();
            T_IG(TRACE_GRP_CALLBACK, "Calling back %s with message length = %u", vtss_module_names[dmodid], msg->len);
            (void)l->filter.cb(l->filter.contxt, msg->usr_msg, msg->len, dmodid, msg->connid);
            T_IG(TRACE_GRP_CALLBACK, "Done calling back %s with message length = %u", vtss_module_names[dmodid], msg->len);
            total = vtss_current_time() - start;
            if (total > dbg_max_rx_callback_ticks[dmodid]) {
                dbg_max_rx_callback_ticks[dmodid] = total;
            }
            handled = TRUE;
            break; // For now, only one cb per dmodid.
        }

        l = l->next;
    }

    MSG_CFG_CRIT_EXIT();

    if (!handled) {
        if (msg->is_tx_msg) {
            // It's a loopback message
            T_WG(TRACE_GRP_RX, "Rx(Msg): Received message (len=%u) to a non-subscribing module (mod=%d=%s, connid=%u)", msg->len, dmodid, vtss_module_names[dmodid], msg->connid);
        } else {
            // It's really received on a port
            T_WG(TRACE_GRP_RX, "Rx(Msg): Received message (len=%u) to a non-subscribing module (mod=%d=%s, seq=[%u,%u], connid=%u)", msg->len, dmodid, vtss_module_names[dmodid], msg->u.rx.left_seq, msg->u.rx.right_seq, msg->connid);
        }
    }

    // Count the event in the right bin.
    if (dmodid > VTSS_MODULE_ID_NONE) {
        // We can only count VTSS_MODULE_ID_NONE+1 modids.
        // The VTSS_MODULE_ID_NONE entry is reserved for
        // modids beyond the pre-allocated.
        dmodid = VTSS_MODULE_ID_NONE;
    }

    // Protect updating the counters, since this function may be called from
    // different threads. Since these counters are global counters, they
    // are never reset, and therefore there're no problems in updating them
    // "outside" the main-state updaters (RX_thread() and msg_topo_event()).
    MSG_COUNTERS_CRIT_ENTER();
    state.usr_stat[msg->state].rx[MIN(dmodid, VTSS_MODULE_ID_NONE)][handled ? 0 : 1]++;
    state.usr_stat[msg->state].rxb[MIN(dmodid, VTSS_MODULE_ID_NONE)] += msg->len;
    MSG_COUNTERS_CRIT_EXIT();
}

/******************************************************************************/
// RX_mac2isid()
// Only valid in master mode.
/******************************************************************************/
static vtss_isid_t RX_mac2isid(mesa_mac_addr_t mac)
{
    vtss_isid_t isid;
    int cid;

    for (isid = MSG_MST_ISID_START_IDX; isid <= MSG_MST_ISID_END_IDX; isid++) {
        for (cid = 0; cid < MSG_CFG_CONN_CNT; cid++) {
            if (memcmp(mcbs[isid][cid].dmac, mac, sizeof(mesa_mac_addr_t)) == 0) {
                // We know that other connections cannot have the same SMAC - even when state
                // is MSG_CONN_STATE_PRI_NO_SEC, because the mcb's dmac is cleared whenever
                // that state is entered.
                return isid;
            }
        }
    }

    return 0; // Invalid ISID.
}

/****************************************************************************/
/*                                                                          */
/*  COMMON INTERNAL FUNCTIONS, PART 2                                       */
/*                                                                          */
/****************************************************************************/

/******************************************************************************/
// CX_thread_post_init()
/******************************************************************************/
static void CX_thread_post_init(void)
{
    mesa_rc rc;

    // Get this switch's MAC address */
    rc = conf_mgmt_mac_addr_get(this_mac, 0);
    VTSS_ASSERT(rc >= 0);

    // Resume the RX_thread
    RX_thread_lock.lock(false);
}

/****************************************************************************/
// TX_handle_tx()
// crit_msg_state taken prior to this call.
/****************************************************************************/
static void TX_handle_tx(msg_mcb_t *mcb, vtss_isid_t disid, u8 cid, vtss_tick_count_t cur_time)
{

    switch (mcb->state) {
    case MSG_CONN_STATE_PRI_NO_SEC:
        // Slave not present. Nothing to do.
        break;

    case MSG_CONN_STATE_PRI_ESTABLISHED:
        if (disid == state.misid) {
            // Loopback!
            // Since we're not allowed to call back the Tx Done User Module
            // callback function while we own the crit_msg_state (as we do here), we
            // transfer the list of messages to the pend_rx_list, which can only
            // be updated by the RX_thread and therefore doesn't need mutex-
            // protection.
            if (mcb->tx_msg_list) {
                RX_put_list(mcb, mcb->tx_msg_list, mcb->tx_msg_list_last);
                mcb->tx_msg_list      = NULL;
                mcb->tx_msg_list_last = NULL;
            }
        } else {
        }
        break;

    case MSG_CONN_STATE_PRI_STOP:
        // Slave doesn't support this CID. Nothing to do.
        break;

    case MSG_CONN_STATE_SEC_NO_PRI:
        break;

    case MSG_CONN_STATE_SEC_WAIT_FOR_MSYNACKS_MACK:
        break;

    case MSG_CONN_STATE_SEC_ESTABLISHED:
        break;

    default:
        VTSS_ASSERT(FALSE);
        break; // Unreachable
    }
}

/****************************************************************************/
// TX_thread()
// Handles connection establishment, transmission, and Tx-done callback for
// non-looped-back messages.
//
// Originally, the TX_thread and RX_thread were one single thread, but it
// turned out to give rise to a deadlock for the following reason:
//   Some user modules have a semaphore that protects one single "reply
//   message". This reply message's semaphore is waited for in the
//   user module's msg RX callback function if the Rx'd message calls for
//   a reply. The semaphore is released in the user module's Tx Done
//   function. If two requests arrived quickly after each other, the
//   first invokation of the RX callback function would acquire the semaphore
//   and Tx a reply. If the second invokation of the RX callback function
//   occurs before the first reply was sent, the RX callback function would
//   wait for the reply buffer semaphore, and because the TxDone was called
//   from the same thread as the Rx callback, the TxDone would never get
//   called, resulting in a deadlock.
//   Now, with two threads, the TxDone will eventually occur, causing the
//   RX callback to be able to continue execution.
//   Even with two threads, the loopback case will still fail, since
//   looped back messages must be RX called back before they can be TxDone.
//   called back. Since there is a strict order on these two events, it
//   doesn't help to move the TxDone call to the Tx thread. The remedy is to
//   implement another another msg_tx option, MSG_TX_OPT_NO_ALLOC_ON_LOOPBACK,
//   which, when not used, causes the msg_tx_adv() function to VTSS_MALLOC() and
//   memcpy the user message, then callback the TxDone function, before
//   actually calling the RX function.
/****************************************************************************/
static void TX_thread(vtss_addrword_t data)
{
    int               i, cid, start_idx, end_idx, msgs_freed;
    msg_item_t        *msg;
    vtss_tick_count_t cur_time;

    // Wait until INIT_CMD_INIT is complete.
    msg_wait(MSG_WAIT_UNTIL_INIT_DONE, VTSS_MODULE_ID_MSG);

    // Load configuration and initialize some global variables.
    CX_thread_post_init();

    // Now, we're ready to accept calls.
    /*lint --e{455} */
    MSG_CFG_CRIT_EXIT();
    MSG_STATE_CRIT_EXIT();
    MSG_COUNTERS_CRIT_EXIT();
    MSG_PEND_LIST_CRIT_EXIT();
    MSG_IM_FIFO_CRIT_EXIT();

    while (1) {
        // Wait until we get an event or we timeout.
        // We timeout every MSG_SAMPLE_TIME_MS msecs.
        (void)vtss_flag_timed_wait(&TX_msg_flag, 0xFFFFFFFF, VTSS_FLAG_WAITMODE_OR_CLR, vtss_current_time() + VTSS_OS_MSEC2TICK(MSG_SAMPLE_TIME_MS));

        // Get the mutex, so that we can safely manipulate the internal state.
        MSG_STATE_CRIT_ENTER();

        // Sample this one once, only and use it throughout the below code
        cur_time = vtss_current_time();

        /*****************************************************************/
        // MUTEX OWNED. DO NOT CALL BACK ANY USER MODULE FUNCTION
        /*****************************************************************/

        // Loop through all the MCBs defined in this state and
        // handle connection establishment, timeout, retransmission, etc.
        if (state.state == MSG_MOD_STATE_PRI) {
            start_idx = MSG_MST_ISID_START_IDX;
            end_idx   = MSG_MST_ISID_END_IDX;
        } else {
            start_idx = MSG_SLV_ISID_IDX;
            end_idx   = MSG_SLV_ISID_IDX;
        }

        for (i = start_idx; i <= end_idx; i++) {
            for (cid = MSG_CFG_CONN_CNT - 1; cid >= 0; cid--) {
                // Start from behind due to the inherited priority
                TX_handle_tx(&mcbs[i][cid], i, cid, cur_time);
            }
        }

        // Release the mutex, so that we can callback User Module functions.
        MSG_STATE_CRIT_EXIT();

        /*****************************************************************/
        // MUTEX RELEASED. NOW CALL BACK USER MODULE FUNCTIONS AS NEEDED
        // BUT BE AWARE WHEN UPDATING PER-CONNECTION OR STATE-DEPENDENT
        // COUNTERS. THIS MAY HAPPEN IN A WRONG BIN, BECAUSE AT THIS EXACT
        // MOMENT IN TIME, A TOPOLOGY CHANGE MAY OCCUR.
        /*****************************************************************/

        msgs_freed = 0;

        // Loop through the list pending to be Tx-done called back.
        while ((msg = CX_get_pend_list(&pend_tx_done_list, &pend_tx_done_list_last)) != NULL) {
            BOOL tx_done = TRUE;

            if (tx_done) {
                // We can safely free it.
                CX_free_msg(msg);
                if (++msgs_freed == 10) {
                    // 10 is arbitrarily chosen. Give TX_handle_tx() a chance to run now,
                    // by signaling to ourselves that we want to run again ASAP.
                    // If we didn't do this, we might end up in a situation, with almost 100%
                    // CPU load spent by other threads that are kept awake by the fact that
                    // we release messages to them but don't actually send messages out,
                    // eventually causing a memory depletion. This is primarily a problem
                    // with looped messages, which may be freed before their Rx handler
                    // is called back (with another copy of the message).
                    TX_wake_up_msg_thread(MSG_FLAG_TX_MORE_WORK);
                    break;
                }
            } //else {
            //     // Put it back into the pend_tx_done_list (in the front)
            //     // and go back to sleep.
            //     TX_put_done_list_front(msg);
            //     break;
            // }
        }
    }
}

/****************************************************************************/
// RX_thread()
// Handles Rx callback for all messages and Tx-done for looped back messages.
/****************************************************************************/
static void RX_thread(vtss_addrword_t data)
{
    msg_item_t *msg;

    // Wait until we get started
    RX_thread_lock.wait();

    while (1) {
        // Wait until we get an event or we timeout.
        // We timeout every MSG_SAMPLE_TIME_MS msecs.
        (void)vtss_flag_wait(&RX_msg_flag, 0xFFFFFFFF, VTSS_FLAG_WAITMODE_OR_CLR);

        // Loop through the list pending to be Rx called back (and
        // Tx-done called back when the Rx is done, in case of loopback).
        while ((msg = CX_get_pend_list(&pend_rx_list, &pend_rx_list_last)) != NULL) {
            // msg is now safely removed from the list.
            RX_msg_dispatch(msg);
            CX_free_msg(msg); // Causes a Tx Done callback if loopback
        }
    }
}

/****************************************************************************/
/*                                                                          */
/*  DEBUG INTERNAL FUNCTIONS                                                */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
// DBG_cmd_syntax_error()
/****************************************************************************/
static void DBG_cmd_syntax_error(msg_dbg_printf_t dbg_printf, const char *fmt, ...)
{
    va_list ap;
    char s[200] = "Command syntax error: ";
    int len;

    len = strlen(s);

    va_start(ap, fmt);

    (void)vsnprintf(s + len, sizeof(s) - len - 1, fmt, ap);
    (void)dbg_printf("%s\n", s);

    va_end(ap);
}

/****************************************************************************/
// DBG_cmd_stat_usr_msg_print()
// cmd_text   : "Print User Message Statistics"
// arg_syntax : NULL,
// max_arg_cnt: 0,
/****************************************************************************/
static void DBG_cmd_stat_usr_msg_print(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    int modid, state_idx, rc_idx;
    BOOL rx_without_subscribers_exists = FALSE;
    BOOL rx_with_subscribers_exists    = FALSE;
    BOOL rc_exists                     = FALSE;
    BOOL rc_exists_per_rc[MSG_TX_RC_LAST_ENTRY];
    BOOL print_this_modid[VTSS_MODULE_ID_NONE + 1];

    memset(print_this_modid, 0, sizeof(print_this_modid));
    memset(rc_exists_per_rc, 0, sizeof(rc_exists_per_rc));

    MSG_COUNTERS_CRIT_ENTER();

    // First figure out what to print
    for (modid = 0; modid <= VTSS_MODULE_ID_NONE; modid++) {
        for (state_idx = 0; state_idx < MSG_MOD_STATE_LAST_ENTRY; state_idx++) {
            if (state.usr_stat[state_idx].rx[modid][0] != 0 || state.usr_stat[state_idx].tx[modid][0] != 0 || state.usr_stat[state_idx].tx[modid][1] != 0) {
                print_this_modid[modid] = TRUE;
                rx_with_subscribers_exists = TRUE;
            }
            if (state.usr_stat[state_idx].rx[modid][1] != 0) {
                rx_without_subscribers_exists = TRUE;
            }
        }
    }

    (void)dbg_printf("Received and transmitted User Messages:\n");
    (void)dbg_printf("                          ---------------------- As Master --------------------- ---------------------- As Slave ----------------------\n");
    (void)dbg_printf("Module                ID  Rx OK      Tx OK      Tx Err     Rx Bytes   Tx Bytes   Rx OK      Tx OK      Tx Err     Rx Bytes   Tx Bytes  \n");
    (void)dbg_printf("--------------------- --- ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- ----------\n");
    if (rx_with_subscribers_exists) {
        for (modid = 0; modid <= VTSS_MODULE_ID_NONE; modid++) {
            if (print_this_modid[modid]) {
                (void)dbg_printf("%-21s %3d %10u %10u %10u %10u %10u %10u %10u %10u %10u %10u\n", vtss_module_names[modid], modid,
                                 state.usr_stat[MSG_MOD_STATE_PRI].rx[modid][0],
                                 state.usr_stat[MSG_MOD_STATE_PRI].tx[modid][0],
                                 state.usr_stat[MSG_MOD_STATE_PRI].tx[modid][1],
                                 state.usr_stat[MSG_MOD_STATE_PRI].rxb[modid],
                                 state.usr_stat[MSG_MOD_STATE_PRI].txb[modid],
                                 state.usr_stat[MSG_MOD_STATE_SEC].rx[modid][0],
                                 state.usr_stat[MSG_MOD_STATE_SEC].tx[modid][0],
                                 state.usr_stat[MSG_MOD_STATE_SEC].tx[modid][1],
                                 state.usr_stat[MSG_MOD_STATE_SEC].rxb[modid],
                                 state.usr_stat[MSG_MOD_STATE_SEC].txb[modid]);
            }
        }
    } else {
        (void)dbg_printf("<none>\n");
    }

    (void)dbg_printf("\nReceived User Messages without subscribers:\n");
    (void)dbg_printf("Module                ID  As Master  As Slave  \n");
    (void)dbg_printf("--------------------- --- ---------- ----------\n");
    if (rx_without_subscribers_exists) {
        for (modid = 0; modid <= VTSS_MODULE_ID_NONE; modid++) {
            if (state.usr_stat[MSG_MOD_STATE_PRI].rx[modid][1] != 0 || state.usr_stat[MSG_MOD_STATE_SEC].rx[modid][1] != 0) {
                (void)dbg_printf("%-21s %3d %10u %10u\n", vtss_module_names[modid], modid, state.usr_stat[MSG_MOD_STATE_PRI].rx[modid][1], state.usr_stat[MSG_MOD_STATE_SEC].rx[modid][1]);
            }
        }
    } else {
        (void)dbg_printf("<none>\n");
    }

    for (rc_idx = 0; rc_idx < MSG_TX_RC_LAST_ENTRY; rc_idx++) {
        if (state.usr_stat[MSG_MOD_STATE_PRI].tx_per_return_code[rc_idx] != 0 || state.usr_stat[MSG_MOD_STATE_SEC].tx_per_return_code[rc_idx] != 0) {
            rc_exists_per_rc[rc_idx] = TRUE;
            rc_exists = TRUE;
        }
    }

    (void)dbg_printf("\nTx Done return code (rc) counters:\n");
    (void)dbg_printf("rc As Master  As Slave\n");
    (void)dbg_printf("-- ---------- ----------\n");
    if (rc_exists) {
        for (rc_idx = 0; rc_idx < MSG_TX_RC_LAST_ENTRY; rc_idx++) {
            if (rc_exists_per_rc[rc_idx]) {
                (void)dbg_printf("%2d %10u %10u\n", rc_idx, state.usr_stat[MSG_MOD_STATE_PRI].tx_per_return_code[rc_idx], state.usr_stat[MSG_MOD_STATE_SEC].tx_per_return_code[rc_idx]);
            }
        }
    } else {
        (void)dbg_printf("<none>\n");
    }

    (void)dbg_printf("\n");
    MSG_COUNTERS_CRIT_EXIT();
}

/****************************************************************************/
// DBG_cmd_stat_protocol_print()
// cmd_text   : "Print Message Protocol Statistics"
// arg_syntax : NULL
// max_arg_cnt: 0
/****************************************************************************/
static void DBG_cmd_stat_protocol_print(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    vtss_isid_t isid;
    int cid;
    int state_idx;
    int pdu_type_idx;

    MSG_STATE_CRIT_ENTER();

    (void)dbg_printf("\nOverall, persistent statistics:\n");
    (void)dbg_printf("PDU Type State  Rx OK      Tx OK      Re-Tx OK   Rx Err     Tx Err     Re-Tx Err \n");
    (void)dbg_printf("-------- ------ ---------- ---------- ---------- ---------- ---------- ----------\n");
    for (state_idx = 0; state_idx < 2; state_idx++) {
        for (pdu_type_idx = MSG_PDU_TYPE_MSYN; pdu_type_idx <= MSG_PDU_TYPE_LAST_ENTRY; pdu_type_idx++) {
            u32 rx_good, tx_good, retx_good, rx_bad, tx_bad, retx_bad;

            rx_good = state.pdu_stat[state_idx].rx[pdu_type_idx][0];
            rx_bad  = state.pdu_stat[state_idx].rx[pdu_type_idx][1];

            if (pdu_type_idx < MSG_PDU_TYPE_LAST_ENTRY) {
                tx_good   = state.pdu_stat[state_idx].tx[pdu_type_idx][0][0];
                retx_good = state.pdu_stat[state_idx].tx[pdu_type_idx][1][0];
                tx_bad    = state.pdu_stat[state_idx].tx[pdu_type_idx][0][1];
                retx_bad  = state.pdu_stat[state_idx].tx[pdu_type_idx][1][1];
            } else {
                // Unknown PDU types are counted in both bin 0 and the last bin.
                rx_good += state.pdu_stat[state_idx].rx[0][0];
                rx_bad  += state.pdu_stat[state_idx].rx[0][1];
                // There's no such thing as Tx'd unknown PDUs
                tx_good   = 0;
                retx_good = 0;
                tx_bad    = 0;
                retx_bad  = 0;
            }
            (void)dbg_printf("%-8s %-6s %10u %10u %10u %10u %10u %10u\n", PDUTYPE2STR(pdu_type_idx), state_idx == MSG_MOD_STATE_SEC ? "Slave" : "Master", rx_good, tx_good, retx_good, rx_bad, tx_bad, retx_bad);
        }
    }

    (void)dbg_printf("\n\nOverall, persistent payload statistics (in bytes):\n");
    (void)dbg_printf("State  Rx OK      Tx OK      Rx Err     Timeouts  \n");
    (void)dbg_printf("------ ---------- ---------- ---------- ----------\n");
    for (state_idx = 0; state_idx < 2; state_idx++) {
        (void)dbg_printf("%-6s " VPRI64Fu("10") " " VPRI64Fu("10") " " VPRI64Fu("10") " %10u\n",
                         state_idx == MSG_MOD_STATE_SEC ? "Slave" : "Master",
                         state.pdu_stat[state_idx].rx_md_bytes[0],
                         state.pdu_stat[state_idx].tx_md_bytes,
                         state.pdu_stat[state_idx].rx_md_bytes[1],
                         state.pdu_stat[state_idx].tx_md_timeouts);
    }

    (void)dbg_printf("\n\nPer-connection, volatile statistics (counted in user messages - not PDUs):\n");
    if (state.state == MSG_MOD_STATE_PRI) {
        (void)dbg_printf("ISID M State      Exists UPSID Established @             Uptime [s] Rx OK     Tx OK       Tx Err     Roundtrip [ms]");
        if (MSG_CFG_CONN_CNT > 1) {
            (void)dbg_printf(" CID");
        }
        (void)dbg_printf("\n");

        (void)dbg_printf("---- - ---------- ------ ----- ------------------------- ---------- ---------- ---------- ---------- --------------");
        if (MSG_CFG_CONN_CNT > 1) {
            (void)dbg_printf(" ---");
        }
        (void)dbg_printf("\n");

        for (isid = MSG_MST_ISID_START_IDX; isid <= MSG_MST_ISID_END_IDX; isid++) {
            for (cid = 0; cid < MSG_CFG_CONN_CNT; cid++) {
                msg_mcb_t *mcb = &mcbs[isid][cid];
                if (mcb->state != MSG_CONN_STATE_PRI_NO_SEC) {
                    (void)dbg_printf("%4d %1s %10s %-6s N/A   ", isid, isid == state.misid ? "Y" : "N", CX_state2str(mcb->state), CX_user_state.exists[isid] ? "Y" : "N");
                    if (mcb->state == MSG_CONN_STATE_PRI_ESTABLISHED) {
                        (void)dbg_printf("%25s ", misc_time2str(mcb->estab_time == 0 ? 1 : mcb->estab_time));
                    } else {
                        (void)dbg_printf("%25s ", "N/A");
                    }
                    (void)dbg_printf("%10u %10u %10u %10u " VPRI64Fu("14"),
                                     CX_uptime_get_crit_taken(isid),
                                     mcb->stat.rx_msg,
                                     mcb->stat.tx_msg[0],
                                     mcb->stat.tx_msg[1],
                                     VTSS_OS_TICK2MSEC(mcb->stat.max_roundtrip_tick_cnt));

                    if (MSG_CFG_CONN_CNT > 1) {
                        (void)dbg_printf(" %3d", cid);
                    }
                    (void)dbg_printf("\n");
                }
            }
        }
    } else {
        (void)dbg_printf("CONID State      UPSID Established @             Uptime [s] Rx OK     Tx OK       Tx Err     Roundtrip [ms]");
        if (MSG_CFG_CONN_CNT > 1) {
            (void)dbg_printf(" CID");
        }
        (void)dbg_printf("\n");
        (void)dbg_printf("----- ---------- ----- ------------------------- ---------- ---------- ---------- ---------- --------------");
        if (MSG_CFG_CONN_CNT > 1) {
            (void)dbg_printf(" ---");
        }
        (void)dbg_printf("\n");
        for (cid = 0; cid < MSG_CFG_CONN_CNT; cid++) {
            msg_mcb_t *mcb = &mcbs[MSG_SLV_ISID_IDX][cid];
            if (mcb->state != MSG_CONN_STATE_SEC_NO_PRI) {
                (void)dbg_printf("%5u %10s N/A   ", mcb->connid, CX_state2str(mcb->state));
                if (mcb->state == MSG_CONN_STATE_SEC_ESTABLISHED) {
                    (void)dbg_printf("%25s ", misc_time2str(mcb->estab_time == 0 ? 1 : mcb->estab_time));
                } else {
                    (void)dbg_printf("%25s ", "N/A");
                }
                (void)dbg_printf("%10u %10u %10u %10u " VPRI64Fu("14"),
                                 CX_uptime_get_crit_taken(VTSS_ISID_LOCAL),
                                 mcb->stat.rx_msg, mcb->stat.tx_msg[0],
                                 mcb->stat.tx_msg[1],
                                 VTSS_OS_TICK2MSEC(mcb->stat.max_roundtrip_tick_cnt));
            }
            if (MSG_CFG_CONN_CNT > 1) {
                (void)dbg_printf(" %3d", cid);
            }
            (void)dbg_printf("\n");
        }
    }

    MSG_STATE_CRIT_EXIT();
}

/****************************************************************************/
// DBG_cmd_stat_last_rx_cb_print()
// cmd_text   : "Print last callback to Msg Rx"
// arg_syntax : NULL
// max_arg_cnt: 0
/****************************************************************************/
static void DBG_cmd_stat_last_rx_cb_print(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    // Typically when this info is needed it's because something is wrong, so
    // we don't take any semaphores before printing this out (the right one to
    // take if we needed one is the MSG_CFG_CRIT_ENTER()).
    (void)dbg_printf("Latest call to Msg Rx callback: mod=%d=%s, len=%u, connid=%u\n", dbg_latest_rx_modid, vtss_module_names[dbg_latest_rx_modid], dbg_latest_rx_len, dbg_latest_rx_connid);
}

/****************************************************************************/
// DBG_cmd_stat_last_im_cb_print()
// cmd_text   : "Print last callback to init_modules()"
// arg_syntax : NULL
// max_arg_cnt: 0
/****************************************************************************/
static void DBG_cmd_stat_last_im_cb_print(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    vtss_init_data_t latest_data;
    const char       *latest_init_module_func_name;
    control_dbg_latest_init_modules_get(&latest_data, &latest_init_module_func_name);
    (void)dbg_printf("Latest call to init_modules(): cmd=%s, isid=%u, flags=0x%x, init-func=%s\n", control_init_cmd2str(latest_data.cmd), latest_data.isid, latest_data.flags, latest_init_module_func_name);
}

/****************************************************************************/
// DBG_cmd_stat_im_max_cb_time_print()
// cmd_text   : "Print longest init_modules() callback time"
// arg_syntax : "[clear]"
// max_arg_cnt: 1
/****************************************************************************/
static void DBG_cmd_stat_im_max_cb_time_print(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    extern void control_dbg_init_modules_callback_time_max_print(msg_dbg_printf_t dbg_printf, BOOL clear);
    control_dbg_init_modules_callback_time_max_print(dbg_printf, parms_cnt == 1 ? TRUE : FALSE);
}

/****************************************************************************/
// DBG_cmd_stat_rx_max_cb_time_print()
// cmd_text   : "Print longest Msg Rx callback time"
// arg_syntax : "[clear]"
// max_arg_cnt: 1
/****************************************************************************/
static void DBG_cmd_stat_rx_max_cb_time_print(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    int i, cnt = 0;

    MSG_CFG_CRIT_ENTER();

    if (parms_cnt == 1) {
        // Clear
        memset(dbg_max_rx_callback_ticks, 0, sizeof(dbg_max_rx_callback_ticks));
        (void)dbg_printf("Msg Rx callback statistics cleared\n");
        goto do_exit;
    }

    (void)dbg_printf("Module                Max time [ms]\n");
    (void)dbg_printf("--------------------- -------------\n");

    for (i = 0; i < ARRSZ(dbg_max_rx_callback_ticks); i++) {
        if (dbg_max_rx_callback_ticks[i]) {
            (void)dbg_printf("%-21s " VPRI64Fu("13") "\n", vtss_module_names[i], VTSS_OS_TICK2MSEC(dbg_max_rx_callback_ticks[i]));
            cnt++;
        }
    }

    if (cnt == 0) {
        (void)dbg_printf("<none called back or time not registrable>\n");
    }

do_exit:
    MSG_CFG_CRIT_EXIT();
}

/****************************************************************************/
// DBG_cmd_stat_pool_print()
// cmd_text   : "Print message pool statistics"
// arg_syntax : NULL
// max_arg_cnt: 0
/****************************************************************************/
static void DBG_cmd_stat_pool_print(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    msg_buf_pool_t *pool_iter;
    MSG_BUF_CRIT_ENTER();
    pool_iter = MSG_buf_pool;

    (void)dbg_printf("Module                Description      Buf size   Max Bufs   Min Bufs   Cur Bufs   Allocs\n");
    (void)dbg_printf("--------------------- ---------------- ---------- ---------- ---------- ---------- ----------\n");
    while (pool_iter) {
        (void)dbg_printf("%-21s %-16s %10u %10u %10u %10u %10u\n", vtss_module_names[pool_iter->module_id], pool_iter->dscr, pool_iter->buf_size, pool_iter->buf_cnt_init, pool_iter->buf_cnt_min, pool_iter->buf_cnt_cur, pool_iter->allocs);
        pool_iter = pool_iter->next;
    }
    MSG_BUF_CRIT_EXIT();
}

/****************************************************************************/
// DBG_cmd_switch_info_print()
// cmd_text   : "Print switch info"
// arg_syntax : NULL
// max_arg_cnt: 0
/****************************************************************************/
static void DBG_cmd_switch_info_print(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    vtss_isid_t             isid;
    msg_flash_switch_info_t local_info;
    BOOL                    local_really_configurable[VTSS_ISID_END];
    BOOL                    is_slave;

    MSG_STATE_CRIT_ENTER();
    local_info = CX_switch_info;
    is_slave = state.state != MSG_MOD_STATE_PRI;
    memcpy(local_really_configurable, CX_switch_really_configurable, sizeof(local_really_configurable));
    MSG_STATE_CRIT_EXIT();

    (void)dbg_printf("ISID USID Configurable Exists Port Count Stack Port 0 Stack Port 1 Board Type API Instance\n");
    (void)dbg_printf("---- ---- ------------ ------ ---------- ------------ ------------ ---------- ------------\n");
    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        init_switch_info_t *info = &local_info.info[isid];
        if (local_really_configurable[isid]) {
            (void)dbg_printf("%4u %4u %-12s %-6s %10u %12u %12u %10u %12x\n",
                             isid,
                             topo_isid2usid(isid),
                             info->configurable ? "Yes" : "(Yes)", // Print in parentheses for slaves in standalone mode
                             is_slave ? "N/A" : msg_switch_exists(isid) ? "Yes" : "No",
                             info->port_cnt,
                             iport2uport(info->stack_ports[0]),
                             iport2uport(info->stack_ports[1]),
                             info->board_type,
                             info->api_inst_id);
        } else {
            (void)dbg_printf("%4u %4s No\n", isid, "N/A");
        }
    }
}

/****************************************************************************/
// DBG_cmd_switch_info_reload()
// cmd_text   : "Reload switch info from flash (has an effect on slaves only)"
// arg_syntax : NULL
// max_arg_cnt: 0
/****************************************************************************/
static void DBG_cmd_switch_info_reload(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    MSG_STATE_CRIT_ENTER();
    CX_flash_read(VTSS_ISID_GLOBAL); // By calling with an invalid ISID, the flash will not be updated in case it contains invalid info.
    MSG_STATE_CRIT_EXIT();
}

/****************************************************************************/
// DBG_cmd_cfg_flash_clear()
// cmd_text   : Erase message module's knowledge about other switches. Takes effect upon next boot.
// arg_syntax : NULL
// max_arg_cnt: 0
/****************************************************************************/
static void DBG_cmd_cfg_flash_clear(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    (void)conf_sec_create(CONF_SEC_GLOBAL, CONF_BLK_MSG, 0);
}

/****************************************************************************/
// DBG_cmd_stat_shaper_print()
// cmd_text   : "Print shaper status"
// arg_syntax : NULL
// max_arg_cnt: 0
/****************************************************************************/
static void DBG_cmd_stat_shaper_print(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    u32 cnt = 0;
    vtss_module_id_t modid;
    MSG_STATE_CRIT_ENTER();
    for (modid = 0; modid < VTSS_MODULE_ID_NONE; modid++) {
        if (msg_shaper[modid].drops) {
            if (cnt++ == 0) {
                (void)dbg_printf("Module                Current    Limit      Drops\n");
                (void)dbg_printf("--------------------- ---------- ---------- ----------\n");
            }
            (void)dbg_printf("%-21s %10u %10u %10u\n", vtss_module_names[modid], msg_shaper[modid].current, msg_shaper[modid].limit, msg_shaper[modid].drops);
        }
    }
    MSG_STATE_CRIT_EXIT();

    if (cnt == 0) {
        (void)dbg_printf("No modules have been shaped so far.\n");
    }
}

/****************************************************************************/
// DBG_cmd_stat_usr_msg_clear()
// cmd_text   : "Clear User Message Statistics",
// arg_syntax : NULL,
// max_arg_cnt: 0,
/****************************************************************************/
static void DBG_cmd_stat_usr_msg_clear(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    MSG_COUNTERS_CRIT_ENTER();
    memset(&state.usr_stat[0], 0, sizeof(state.usr_stat));
    MSG_COUNTERS_CRIT_EXIT();

    (void)dbg_printf("User Message statistics cleared!\n");
}

/****************************************************************************/
// DBG_cmd_stat_protocol_clear()
// cmd_text   : "Clear Protocol Statistics",
// arg_syntax : NULL,
// max_arg_cnt: 0,
/****************************************************************************/
static void DBG_cmd_stat_protocol_clear(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    vtss_isid_t isid;
    int cid;

    // Reset statistics

    MSG_STATE_CRIT_ENTER();

    // Connection statistics
    for (isid = 0; isid < VTSS_ISID_CNT + 1; isid++) {
        for (cid = 0; cid < MSG_CFG_CONN_CNT; cid++) {
            memset(&mcbs[isid][cid].stat, 0, sizeof(mcbs[isid][cid].stat));
        }
    }

    // Master/slave statistics
    memset(&state.pdu_stat[0], 0, sizeof(state.pdu_stat));

    // Relay statistics
    memset(&state.relay_stat[0], 0, sizeof(state.relay_stat));

    MSG_STATE_CRIT_EXIT();

    (void)dbg_printf("Message protocol statistics cleared!\n");
}

/****************************************************************************/
// DBG_cmd_cfg_trace_isid_set()
// cmd_text   : "Configure trace output - only used when master"
// arg_syntax : "[<isid> [<enable> (0 or 1)]] - Use <isid> = 0 to enable or disable all"
// max_arg_cnt: 2
/****************************************************************************/
static void DBG_cmd_cfg_trace_isid_set(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    vtss_isid_t isid;
    if (parms_cnt == 0) {
        (void)dbg_printf("ISID Trace Enabled\n");
        (void)dbg_printf("---- -------------\n");
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            (void)dbg_printf("%4d %d\n", isid, msg_trace_enabled_per_isid[isid]);
        }
        return;
    }

    if (parms_cnt != 2) {
        DBG_cmd_syntax_error(dbg_printf, "This function takes 0 or 2 arguments, not %d", parms_cnt);
        return;
    }

    if (parms[0] >= VTSS_ISID_END) {
        DBG_cmd_syntax_error(dbg_printf, "ISIDs must reside in interval [%d; %d] or be 0 (meaning change all)", VTSS_ISID_START, VTSS_ISID_END - 1);
        return;
    }

    if (parms[0] == 0) {
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            msg_trace_enabled_per_isid[isid] = parms[1];
        }
    } else {
        msg_trace_enabled_per_isid[parms[0]] = parms[1];
    }
}

/****************************************************************************/
// DBG_cmd_cfg_trace_modid_set()
// cmd_text   : "Configure trace output per module"
// arg_syntax : "[<module_id> [<enable> (0 or 1)]] - Use <module_id> = -1 to enable or disable all"
// max_arg_cnt: 2
/****************************************************************************/
static void DBG_cmd_cfg_trace_modid_set(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    vtss_module_id_t modid;
    if (parms_cnt == 0) {
        (void)dbg_printf("Module                ID  Trace Enabled\n");
        (void)dbg_printf("--------------------- --- -------------\n");
        for (modid = 0; modid < VTSS_MODULE_ID_NONE; modid++) {
            (void)dbg_printf("%-21s %3d %d\n", vtss_module_names[modid], modid, msg_trace_enabled_per_modid[modid]);
        }
        return;
    }

    if (parms_cnt != 2) {
        DBG_cmd_syntax_error(dbg_printf, "This function takes 0 or 2 arguments, not %d", parms_cnt);
        return;
    }

    if (parms[0] != (u32)(-1) && parms[0] >= VTSS_MODULE_ID_NONE) {
        DBG_cmd_syntax_error(dbg_printf, "Module IDs must reside in interval [0; %d] or be -1 (meaning change all)", VTSS_MODULE_ID_NONE - 1);
        return;
    }

    if (parms[0] == (u32)(-1)) {
        for (modid = 0; modid < VTSS_MODULE_ID_NONE; modid++) {
            msg_trace_enabled_per_modid[modid] = parms[1];
        }
    } else {
        msg_trace_enabled_per_modid[parms[0]] = parms[1];
    }
}

/****************************************************************************/
// DBG_cmd_cfg_timeout()
// cmd_text   : "Configure timeouts"
// arg_syntax : "[<MSYN timeout> [<MD timeout>]] - all in milliseconds (0 = no change)"
// max_arg_cnt: 2
/****************************************************************************/
static void DBG_cmd_cfg_timeout(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    int i;

    MSG_STATE_CRIT_ENTER();

    if (parms_cnt == 0) {
        (void)dbg_printf("PDU Type Timeout [ms]\n");
        (void)dbg_printf("-------- ------------\n");
        (void)dbg_printf("MSYN     %5u\n", state.msg_cfg_msyn_timeout_ms);
        (void)dbg_printf("MD       %5u\n", state.msg_cfg_md_timeout_ms);
        goto do_exit;
    }

    if (parms_cnt != 2) {
        DBG_cmd_syntax_error(dbg_printf, "This function takes 0 or 2 arguments, not %d", parms_cnt);
        goto do_exit;
    }

    for (i = 0; i < 2; i++) {
        if (parms[i] != 0  && (parms[i] < MSG_CFG_MIN_TIMEOUT_MS || parms[i] > MSG_CFG_MAX_TIMEOUT_MS)) {
            DBG_cmd_syntax_error(dbg_printf, "Timeouts must reside in interval [%d; %d] or be 0 (meaning no change)", MSG_CFG_MIN_TIMEOUT_MS, MSG_CFG_MAX_TIMEOUT_MS);
            goto do_exit;
        }
    }

    if (parms[0] != 0) {
        state.msg_cfg_msyn_timeout_ms = parms[0];
    }
    if (parms[2] != 0) {
        state.msg_cfg_md_timeout_ms = parms[2];
    }
    (void)dbg_printf("Changes take effect immediately!\n");

do_exit:
    MSG_STATE_CRIT_EXIT();
}

/****************************************************************************/
// DBG_cmd_cfg_retransmit()
// cmd_text   : "Configure retransmits"
// arg_syntax : "[<MD retransmits>]"
// max_arg_cnt: 1
/****************************************************************************/
static void DBG_cmd_cfg_retransmit(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    MSG_STATE_CRIT_ENTER();

    if (parms_cnt == 0) {
        (void)dbg_printf("PDU Type Retransmits\n");
        (void)dbg_printf("-------- -----------\n");
        (void)dbg_printf("MD       %11u\n", state.msg_cfg_md_retransmit_limit);
        goto do_exit;
    }

    if (parms[0] > MSG_CFG_MAX_RETRANSMIT_LIMIT) {
        DBG_cmd_syntax_error(dbg_printf, "Retransmit limit must reside in interval [0; %d]", MSG_CFG_MAX_RETRANSMIT_LIMIT);
        goto do_exit;
    }

    state.msg_cfg_md_retransmit_limit = parms[0];
    (void)dbg_printf("Changes take effect on next connection negotiation!\n");

do_exit:
    MSG_STATE_CRIT_EXIT();
}

/****************************************************************************/
// DBG_cmd_cfg_winsz()
// cmd_text   : "Configure window sizes"
// arg_syntax : "[[<master per slave> [<slave>]] (0 = no change)",
// max_arg_cnt: 2
/****************************************************************************/
static void DBG_cmd_cfg_winsz(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    int i;

    MSG_STATE_CRIT_ENTER();

    if (parms_cnt == 0) {
        (void)dbg_printf("MWINSZ: %3u\n", state.msg_cfg_pri_winsz_per_sec);
        (void)dbg_printf("SWINSZ: %3u\n", state.msg_cfg_sec_winsz);
        goto do_exit;
    }

    if (parms_cnt != 2) {
        DBG_cmd_syntax_error(dbg_printf, "This function takes 0 or 2 arguments, not %d", parms_cnt);
        goto do_exit;
    }

    for (i = 0; i < 2; i++) {
        if (parms[i] != 0  && (parms[i] < MSG_CFG_MIN_WINSZ || parms[i] > MSG_CFG_MAX_WINSZ)) {
            DBG_cmd_syntax_error(dbg_printf, "Window sizes must reside in interval [%d; %d] or be 0 (meaning no change)", MSG_CFG_MIN_WINSZ, MSG_CFG_MAX_WINSZ);
            goto do_exit;
        }
    }

    if (parms[0] != 0) {
        state.msg_cfg_pri_winsz_per_sec = parms[0];
    }
    if (parms[1] != 0) {
        state.msg_cfg_sec_winsz = parms[1];
    }
    (void)dbg_printf("Changes take effect on next connection negotiation.!\n");

do_exit:
    MSG_STATE_CRIT_EXIT();
}

/****************************************************************************/
// DBG_cmd_test_renegotiate()
// cmd_text   : "Re-negotiate a connection. ISID argument is not needed on slave, and optional on master"
// arg_syntax : "[<ISID>] - leave argument out to re-negotiate all (master only)"
// max_arg_cnt: 1
/****************************************************************************/
static void DBG_cmd_test_renegotiate(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    int min_idx, max_idx, idx, cid;

    MSG_STATE_CRIT_ENTER();

    if (state.state == MSG_MOD_STATE_PRI) {
        // Master
        if (parms_cnt == 0) {
            min_idx = VTSS_ISID_START;
            max_idx = VTSS_ISID_END - 1;
        } else {
            if (parms[0] < VTSS_ISID_START || parms[0] >= VTSS_ISID_END) {
                DBG_cmd_syntax_error(dbg_printf, "ISIDs must reside in interval [%d; %d]", VTSS_ISID_START, VTSS_ISID_END - 1);
                goto do_exit;
            }

            min_idx = max_idx = parms[0];
        }
    } else {
        // Slave
        if (parms_cnt != 0) {
            DBG_cmd_syntax_error(dbg_printf, "Don't use arguments when slave");
            goto do_exit;
        }

        min_idx = max_idx = MSG_SLV_ISID_IDX;
    }

    for (idx = min_idx; idx <= max_idx; idx++) {
        if (idx != state.misid) { // We cannot re-negotiate the loopback connection
            for (cid = 0; cid < MSG_CFG_CONN_CNT; cid++) {
                if ((state.state == MSG_MOD_STATE_PRI && mcbs[idx][cid].state == MSG_CONN_STATE_PRI_ESTABLISHED) ||
                    (state.state == MSG_MOD_STATE_SEC && mcbs[idx][cid].state == MSG_CONN_STATE_SEC_ESTABLISHED)) {
                    CX_restart(&mcbs[idx][cid], idx, cid, MSG_TX_RC_WARN_SLV_DBG_RENEGOTIATION, MSG_TX_RC_WARN_MST_DBG_RENEGOTIATION);
                }
            }
        }
    }

do_exit:
    MSG_STATE_CRIT_EXIT();
}

/****************************************************************************/
// DBG_cmd_test_master_down_up()
// cmd_text   : "Send SWITCH_DEL(<ourselves>), MASTER_DOWN, MASTER_UP, SWITCH_ADD(<ourselves> into init_modules()",
// arg_syntax : NULL
// max_arg_cnt: 0
/****************************************************************************/
static void DBG_cmd_test_master_down_up(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    vtss_isid_t misid;

    MSG_STATE_CRIT_ENTER();

    misid = state.misid;

    if (state.state != MSG_MOD_STATE_PRI) {
        (void)dbg_printf("Must be master\n");
        MSG_STATE_CRIT_EXIT();
        return;
    }

    MSG_STATE_CRIT_EXIT();
    msg_topo_event(MSG_TOPO_EVENT_MASTER_DOWN, 0);
    msg_topo_event(MSG_TOPO_EVENT_MASTER_UP, misid);
    msg_topo_event(MSG_TOPO_EVENT_SWITCH_ADD, misid);
}

/****************************************************************************/
// CX_wait_until_to_str()
/****************************************************************************/
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO)
static const char *CX_wait_until_to_str(msg_wait_until_t what)
{
    switch (what) {
    case MSG_WAIT_UNTIL_INIT_DONE:
        return "init done";

    case MSG_WAIT_UNTIL_ICFG_LOADING_PRE:
        return "icfg loading pre";

    case MSG_WAIT_UNTIL_ICFG_LOADING_POST:
        return "icfg loading post";

    case MSG_WAIT_UNTIL_ICFG_LOADED_POST:
        return "icfg loaded post";

    default:
        return "Unknown #what";
    }
}
#endif

/****************************************************************************/
/*                                                                          */
/*  MODULE EXTERNAL FUNCTIONS                                               */
/*                                                                          */
/****************************************************************************/

/******************************************************************************/
// msg_topo_event()
// Called back by the topology module when a change in the stack occurs.
// Do not make this static, as it is called from msg_test module.
/******************************************************************************/
void msg_topo_event(msg_topo_event_t topo_event, vtss_isid_t new_isid)
{
    int                cid;
    vtss_isid_t        isid;
    mesa_mac_addr_t    mac;
    mesa_rc            rc;
    vtss_init_data_t   event;
    CX_event_init(&event);

    switch (topo_event) {
    case MSG_TOPO_EVENT_CONF_DEF:
        // The easiest way for Topo to serialize switch delete and restore configuration
        // defaults is to let it go through the Message Module.
        // Once this event occurs, the user has explicitly deleted the switch through management.
        T_IG(TRACE_GRP_TOPO, "Got Conf Default Event, ISID = %d", new_isid);
        MSG_ASSERT(new_isid >= VTSS_ISID_START && new_isid < VTSS_ISID_END, "Invalid new_isid: %d", (int)new_isid);

        MSG_STATE_CRIT_ENTER();

        // Cannot receive this event unless we're master.
        VTSS_ASSERT(state.state == MSG_MOD_STATE_PRI);

        // The switch pointed to by new_isid must not currently be in the stack.
        for (cid = 0; cid < MSG_CFG_CONN_CNT; cid++) {
            VTSS_ASSERT(mcbs[new_isid][cid].state == MSG_CONN_STATE_PRI_NO_SEC);
        }

#if !defined(VTSS_SW_OPTION_MSG_HOMOGENEOUS_STACK)
        // If a stack is homogeneous, all switches are 100% alike and therefore always configurable.
        // The switch no longer exists and is not configurable anymore.
        CX_switch_info.info[new_isid].configurable = FALSE;
        CX_switch_really_configurable[new_isid]    = FALSE;
        CX_flash_write();
#endif

        // Generate a CONF-DEF event.
        (void)CX_event_conf_def(new_isid, &CX_switch_info.info[new_isid]);

        MSG_STATE_CRIT_EXIT();
        break;

    case MSG_TOPO_EVENT_MASTER_UP:
        T_IG(TRACE_GRP_TOPO, "Got Master Up Event, ISID = %d", new_isid);
        MSG_ASSERT(new_isid >= VTSS_ISID_START && new_isid < VTSS_ISID_END, "Invalid new_isid: %d", (int)new_isid);

        MSG_STATE_CRIT_ENTER();

        // Cannot receive this while we're already master.
        VTSS_ASSERT(state.state != MSG_MOD_STATE_PRI);

        // Release all pending User Messages on the slave interface.
        for (cid = 0; cid < MSG_CFG_CONN_CNT; cid++) {
            CX_mcb_flush(&mcbs[MSG_SLV_ISID_IDX][cid], MSG_TX_RC_WARN_SLV_MASTER_UP);
        }

        // Return the slave MCBs to their initial state.
        for (cid = 0; cid < MSG_CFG_CONN_CNT; cid++) {
            mcbs[MSG_SLV_ISID_IDX][cid].state = MSG_CONN_STATE_SEC_NO_PRI;
        }

        // Move the master MCBs to the master idle state.
        for (isid = MSG_MST_ISID_START_IDX; isid <= MSG_MST_ISID_END_IDX; isid++) {
            for (cid = 0; cid < MSG_CFG_CONN_CNT; cid++) {
                CX_set_state_mst_no_slv(&mcbs[isid][cid]);
            }
        }

        state.state = MSG_MOD_STATE_PRI;
        state.misid = new_isid;

        event.cmd  = INIT_CMD_ICFG_LOADING_PRE;
        event.isid = 0;

        // Load board IDs and stack ports from flash, so that module configuration can
        // be applied correctly from ICFG once the master-up event has been through the port module.
        CX_flash_read(new_isid);
        memcpy(event.switch_info, &CX_switch_info.info[0], sizeof(event.switch_info));

        // Notify user modules of master up.
        (void)IM_fifo_put(&event);

        MSG_STATE_CRIT_EXIT();
        break;

    case MSG_TOPO_EVENT_MASTER_DOWN:
        T_IG(TRACE_GRP_TOPO, "Got Master Down Event");

        MSG_STATE_CRIT_ENTER();

        // Can't get master down unless we're already master
        VTSS_ASSERT(state.state == MSG_MOD_STATE_PRI);

        // Release all connections towards active slaves
        for (isid = MSG_MST_ISID_START_IDX; isid <= MSG_MST_ISID_END_IDX; isid++) {
            // To ensure proper shut-down, call init_modules() telling them that a slave
            // is on its way out. The init_modules() will also be called for the master itself.
            if (mcbs[isid][0].state == MSG_CONN_STATE_PRI_ESTABLISHED) {
                event.cmd  = INIT_CMD_SWITCH_DEL;
                event.isid = isid;
                (void)IM_fifo_put(&event);
            }
            for (cid = 0; cid < MSG_CFG_CONN_CNT; cid++) {
                CX_mcb_flush(&mcbs[isid][cid], MSG_TX_RC_WARN_MST_MASTER_DOWN);
                CX_set_state_mst_no_slv(&mcbs[isid][cid]);
            }
        }

        // Move the slave MCBs to the ready state.
        for (cid = 0; cid < MSG_CFG_CONN_CNT; cid++) {
            mcbs[MSG_SLV_ISID_IDX][cid].state = MSG_CONN_STATE_SEC_NO_PRI;
        }

        // We ignore MSYNs until the master-down event has been passed to all user-modules.
        CX_user_state.ignore_msyns = TRUE;

        state.state = MSG_MOD_STATE_SEC;

        event.cmd  = INIT_CMD_MASTER_DOWN;
        event.isid = 0;

        // Also send the master down event
        (void)IM_fifo_put(&event);

        MSG_STATE_CRIT_EXIT();
        break;

    case MSG_TOPO_EVENT_SWITCH_ADD:
        T_IG(TRACE_GRP_TOPO, "Got Switch Add Event, ISID = %d", new_isid);

        MSG_ASSERT(new_isid >= VTSS_ISID_START && new_isid < VTSS_ISID_END, "Invalid new_isid: %d", (int)new_isid);
        MSG_STATE_CRIT_ENTER();

        // We must be master to add switches.
        VTSS_ASSERT(state.state == MSG_MOD_STATE_PRI);

        // Check that no other connections use the same MAC address.
        rc = topo_isid2mac(new_isid, mac);
        if (rc < 0) {
            T_W("topo_isid2mac() failed (rc=%d). Topology change on its way?", rc);
            // We need to add the switch anyway, because Topo may give us a switch
            // delete in just a second.
        } else {
            // See if that MAC address is already in use in one of our connections.
            isid = RX_mac2isid(mac);
            MSG_ASSERT(isid == 0, "Switch Add (ISID=%d): A connection with that slave MAC address is already in use on ISID=%d", new_isid, isid);

            // If the new switch is the master, check that topo thinks the MAC address
            // is the same as what we think we have.
            if (new_isid == state.misid) {
                if (memcmp(mac, this_mac, sizeof(mesa_mac_addr_t)) != 0) {
                    T_E("Topo and Conf use different local MAC addresses");
                }
            }
        }

        for (cid = 0; cid < MSG_CFG_CONN_CNT; cid++) {
            // It's illegal to add an already added switch.
            VTSS_ASSERT(mcbs[new_isid][cid].state == MSG_CONN_STATE_PRI_NO_SEC);

            // If we're adding ourselves (the master), go directly to the established state.
            if (new_isid == state.misid) {
                CX_local_switch_info_get(&mcbs[new_isid][cid].u.primary_switch.switch_info);
                mcbs[new_isid][cid].estab_time = time(NULL);
                mcbs[new_isid][cid].state = MSG_CONN_STATE_PRI_ESTABLISHED;
            } else {
                mcbs[new_isid][cid].state = MSG_CONN_STATE_PRI_RDY;
                // Trigger transmission of an MSYN to the slave right away.
                TX_wake_up_msg_thread(MSG_FLAG_TX_MSYN);
            }

            // Reset the statistics
            memset(&mcbs[new_isid][cid].stat, 0, sizeof(mcbs[new_isid][cid].stat));
        }

        // If this is the master ISID, tell switches right away that there's a slave (ourselves),
        // otherwise this will be called after MSYN/MSYNACK negotiation.
        if (new_isid == state.misid) {
            if (CX_switch_info.info[new_isid].configurable == FALSE) {
                // Seems that something went wrong during read from flash.
                T_E("Something fundamental wrong with flash load");
                CX_init_switch_info_get(&CX_switch_info.info[new_isid]);
            }

            if (!CX_event_switch_add(new_isid, &CX_switch_info.info[new_isid])) {
                T_E("Something fundamental wrong with the IM FIFO");
            }
        }

        MSG_STATE_CRIT_EXIT();
        break;

    case MSG_TOPO_EVENT_SWITCH_DEL:
        T_IG(TRACE_GRP_TOPO, "Got Switch Delete Event, ISID = %d", new_isid);

        MSG_ASSERT(new_isid >= VTSS_ISID_START && new_isid < VTSS_ISID_END, "Invalid isid: %d", (int)new_isid);

        MSG_STATE_CRIT_ENTER();

        // We must be master!
        VTSS_ASSERT(state.state == MSG_MOD_STATE_PRI);

        // We cannot remove the master from the list of active switches.
        // If we need to support this, some changes must be done to the msg_tx() function.
        VTSS_ASSERT(new_isid != state.misid);

        // Release all pending User Messages sent to the slave.
        for (cid = 0; cid < MSG_CFG_CONN_CNT; cid++) {
            CX_mcb_flush(&mcbs[new_isid][cid], MSG_TX_RC_WARN_MST_SLAVE_DOWN);
        }

        // Notify User Modules if the switch.
        if (mcbs[new_isid][0].state == MSG_CONN_STATE_PRI_ESTABLISHED) {
            event.cmd  = INIT_CMD_SWITCH_DEL;
            event.isid = new_isid;
            (void)IM_fifo_put(&event);
        }

        for (cid = 0; cid < MSG_CFG_CONN_CNT; cid++) {
            // The slave switch state from the master's point of view must be 'existing',
            // i.e. previously added.
            VTSS_ASSERT(mcbs[new_isid][cid].state != MSG_CONN_STATE_PRI_NO_SEC);
            CX_set_state_mst_no_slv(&mcbs[new_isid][cid]); // No longer present
        }

        MSG_STATE_CRIT_EXIT();
        break;

    default:
        MSG_ASSERT(FALSE, "Unsupported topology change (%d)", topo_event);
        break;
    }
}

/******************************************************************************/
// msg_tx_adv()
/******************************************************************************/
void msg_tx_adv(const void *const contxt, const msg_tx_cb_t cb, msg_tx_opt_t opt, vtss_module_id_t dmodid, u32 did, const void *const msg, size_t len)
{
    msg_tx_rc_t rc = MSG_TX_RC_OK;
    int         cid = 0, i;
    msg_item_t  *usr_msg_item, *tx_done_usr_msg_item;
    msg_mcb_t   *mcb = NULL;
    void        *copy_of_msg;
    u32         did_orig = did;

    MSG_ASSERT(len > 0 && len <= MSG_MAX_LEN_BYTES, "Invalid len = %zu. dmodid = %s", len, vtss_module_names[dmodid]);
    MSG_ASSERT(dmodid < VTSS_MODULE_ID_NONE, "No such module (id = %d)", dmodid);

    MSG_STATE_CRIT_ENTER();
    if (CX_user_state.master) {
        // The user modules are supposed to think that we are master.
        // Check to see if we indeed are:
        if (state.state != MSG_MOD_STATE_PRI) {
            // No we aren't. We must have gotten a master down in the meanwhile.
            rc = MSG_TX_RC_WARN_MST_MASTER_DOWN;
            goto exit_func;
        }

        // In master mode, the @did is supposed to be an ISID, but if the User Module
        // thinks we are (still) a slave, it may be out of bounds.
        if (did > MSG_MST_ISID_END_IDX) {
            rc = MSG_TX_RC_WARN_MST_INV_DISID;
            goto exit_func;
        }

        // If the @did is 0, the message must be looped back.
        if (did == 0) {
            did = state.misid;
        }

        // Locate the MCB.
        if (opt & MSG_TX_OPT_PRIO_HIGH) {
            // Currently this code supports at most 2 connections per slave.
            // The only place to change to support more connections is in the
            // API call to msg_tx_adv(), where the @opt member could take more
            // priorities - and here.
            cid = MSG_CFG_CONN_CNT - 1;
        } else {
            cid = 0;
        }

        // If the user module haven't yet heard that the slave exists,
        // we should not send any messages to it, unless the caller
        // just want to send to current master (#did_orig == 0).
        if (did_orig != 0 && !CX_user_state.exists[did]) {
            rc = MSG_TX_RC_WARN_MST_NO_SLV;
            goto exit_func;
        }

        // Locate the highest indexed MCB with connection index <= cid, that
        // is in the established state.
        mcb = NULL;
        for (i = cid; i >= 0; i--) {
            if (mcbs[did][i].state == MSG_CONN_STATE_PRI_ESTABLISHED) {
                mcb = &mcbs[did][i];
                break;
            }
        }

        if (mcb == NULL) {
            // No established connections found. This could be because
            // the slave just left the stack, without the User Module
            // has yet been notified about it. Flag it as a warning.
            rc = MSG_TX_RC_WARN_MST_NO_SLV;
            goto exit_func;
        }
    } else {

        // The user modules are supposed to think that we are slave.
        // Check to see if we indeed are:
        if (state.state != MSG_MOD_STATE_SEC) {
            // No we aren't. We must have gotten a master up event in the meanwhile.
            rc = MSG_TX_RC_WARN_SLV_MASTER_UP;
        }

        // In slave mode, the @did is supposed to be a connid. connids and ISIDs
        // are disjunct. If we just changed state from master to slave, but the
        // user module isn't aware of this yet, he might attempt to transmit to
        // a certain slave. Therefore we can only flag this as a warning.
        if (did >= MSG_MST_ISID_START_IDX && did <= MSG_MST_ISID_END_IDX) {
            rc = MSG_TX_RC_WARN_SLV_INV_DCONNID;
            goto exit_func;
        }

        // For slaves the MSG_TX_OPT_PRIO_HIGH is ignored, because solicited
        // messages are transmitted on a certain CID, which intrinsically is
        // a priority, and unsolicited messages need not be sent with high
        // prio.
        // When running tests in slave mode, the msg_test module attempts
        // to set this flag, so we cannot assert it's not being used.

        // If @did is 0, use the lowest numbered connection
        if (did == 0) {
            mcb = &mcbs[MSG_SLV_ISID_IDX][0];
        } else {
            // Locate the connection with the given connid.
            mcb = NULL;
            for (i = 0; i < MSG_CFG_CONN_CNT; i++) {
                if (mcbs[MSG_SLV_ISID_IDX][i].connid == did) {
                    mcb = &mcbs[MSG_SLV_ISID_IDX][i];
                    break;
                }
            }
        }

        if (mcb == NULL || mcb->state != MSG_CONN_STATE_SEC_ESTABLISHED) {
            // No established connections found. This could be because
            // of a master up event, or change of master, or simply
            // because of the user module sending an unsolicited
            // message and there's yet no connection established with the
            // master. The return value only flags a warning, since this
            // is "normal" behavior of a changing stack.
            rc = MSG_TX_RC_WARN_SLV_NO_MST;
            goto exit_func;
        }
    }

exit_func:

    // Allocate a structure for holding the properties
    VTSS_MALLOC_CAST(usr_msg_item, sizeof(msg_item_t));
    // It's impossible to pass an out-of-memory on to the message thread,
    // since we need the usr_msg_item to be able to do that.
    VTSS_ASSERT(usr_msg_item);

    // Shape the message if subject to shaping.
    if (opt & MSG_TX_OPT_SHAPE) {
        // Always count, since we always decrement in tx done if #opt contains MSG_TX_OPT_SHAPE
        msg_shaper[dmodid].current++;

        // But only report it as a shaper-drop if nothing else is wrong.
        if (rc == MSG_TX_RC_OK && msg_shaper[dmodid].limit && msg_shaper[dmodid].current > msg_shaper[dmodid].limit) {
            msg_shaper[dmodid].drops++;
            rc = MSG_TX_RC_WARN_SHAPED;
        }
    }

    // If loopback messages were dispatched to the destination module from
    // this function, we might end up in a deadlock if both the module's
    // tx and rx function acquires the same mutex. Therefore, we send
    // loopback messages the same way as other messages, i.e. through
    // the RX_thread.
    // Likewise, if warnings or errors were dispatched back to the user-
    // defined callback from this function, we could end-up in a deadlock
    // if both the module's tx and tx_done function acquires the same mutex.
    // Even if the callback is NULL, and an error occurred, we will pass
    // the call to the TX_thread, because it also increases the statistics,
    // and possibly deletes the memory used by the user message.

    // Fill it
    usr_msg_item->is_tx_msg    = TRUE;
    usr_msg_item->usr_msg      = (u8 *)msg;
    usr_msg_item->len          = len;
    usr_msg_item->dmodid       = dmodid;
    usr_msg_item->connid       = did; // Only used in loopback case (i.e. when we're master).
    usr_msg_item->state        = state.state; // To be able to count in correct statistics bin.
    usr_msg_item->u.tx.opt     = opt;
    usr_msg_item->u.tx.cb      = cb;
    usr_msg_item->u.tx.contxt  = (void *)contxt;
    usr_msg_item->u.tx.rc      = rc;
    usr_msg_item->next         = NULL;

    // In case of an error, we put the message on the pend_tx_done_list, which
    // is absorbed by the TX_thread(). It will eventually call-back any
    // user-defined callback function from outside the user's own thread.
    if (rc != MSG_TX_RC_OK) {
        // Error! Whether it's a loopback message or not, simply send it to the
        // Tx Done list. It never ends up in the RX list.
        TX_put_done_list(NULL, usr_msg_item, usr_msg_item);
    } else {
        // If this is not a loop-back message, create the MDs needed for it, so that they're
        // ready to be transmitted in the TX_thread().
        if (state.state != MSG_MOD_STATE_PRI || did != state.misid) {
            /* This is not a loop-back message */
            if (MSG_TRACE_ENABLED(did, dmodid)) {
                T_IG(TRACE_GRP_TX, "Non-loopback message in a non-stackable system!!!!!");
            }
        } else {
            // It's a loopback message. We need to consider the MSG_TX_OPT_NO_ALLOC_ON_LOOPBACK
            // option.
            if (opt & MSG_TX_OPT_NO_ALLOC_ON_LOOPBACK) {
                // Do not allocate a new user message. Instead the callbacks must be done
                // as follows: RX, TxDone, then perhaps free original user message.
                // This option will be satisfied by simply transferring the message
                // directly to the mcb's tx_msg_list, so that it is processed in order.
                // The TX_handle_tx() will in turn move it to the rx_msg_list, which
                // in turn will call RX followed by TxDone callbacks. These two callbacks
                // will occur from the same thread, namely the RX_thread.
            } else {
                // Allocate a new user message and make a copy of @msg into it.
                // The order of events is:
                //   Concurrently call TxDone(@msg) and Rx callbacks(copy_of_msg).
                //   After TxDone(@msg), free @msg if told to.
                //   After Rx(copy_of_msg), free copy_of_msg.
                // The TxDone will occur from the TX_thread and the RX will occur
                // from the RX_thread.
                // Due to this possible TxDone before Rx behavior, it might happen that
                // a topology change (MASTER_DOWN) causes the RX never to occur even though
                // the TxDone() return code said OK.

                // The currently filled usr_msg_item is actually the one we're going to send
                // to the TxDone callback.
                tx_done_usr_msg_item = usr_msg_item;

                // We need to create a new one containing the one to send to the Rx callback.
                VTSS_MALLOC_CAST(usr_msg_item, sizeof(msg_item_t));
                VTSS_ASSERT(usr_msg_item);

                // Allocate new msg and copy the original @msg to that one.
                copy_of_msg = VTSS_MALLOC(len);
                VTSS_ASSERT(copy_of_msg);
                /* Suppress Lint Warning 670: Possible access beyond array for function 'memcpy(void *, const void *, unsigned int)', argument 3 (size=16777215) exceeds argument 2 (size=360464) */
                /*lint -e{670} */
                memcpy(copy_of_msg, msg, len);

                // Fill the new usr_msg_item with the copy.
                // Disguise it as an Rx message, so that it doesn't get counted twice as TxDone.
                // The only "problem" with that is that the RX_msg_dispatch() function prints a different
                // debug trace for loopback messages than for non-loopback messages.
                usr_msg_item->is_tx_msg  = FALSE; // Disguise it as an Rx message, so that it doesn't get counted twice as TxDone.
                usr_msg_item->usr_msg    = (u8 *)copy_of_msg;
                usr_msg_item->len        = len;
                usr_msg_item->dmodid     = dmodid;
                usr_msg_item->connid     = did;         // Only used in loopback case (i.e. when we're master).
                usr_msg_item->state      = state.state; // To be able to count in correct statistics bin.
                usr_msg_item->u.rx.left_seq = usr_msg_item->u.rx.right_seq = usr_msg_item->u.rx.frags_received = 0; // Only used in trace output.
                usr_msg_item->next       = NULL;

                // Now that we've filled in the copy, we can safely store the tx_done_usr_msg_item in the tx_done list...
                // This also causes a count in "Tx OK"
                TX_put_done_list(mcb, tx_done_usr_msg_item, tx_done_usr_msg_item); // Both this and the TX_wake_up_msg_thread() call below will wake up the message thread, but it doesn't really matter.

                // ... and fall out of this branch. The usr_msg_item is now correct and ready to be
                // stored in the tx_msg_list! The loopback is detected by the TX_handle_tx() and
                // results in a transfer to the RX_msg_list. There's no good reason for sending looped
                // back messages across the TX_thread, other than it simplifies this function.
            }

            if (MSG_TRACE_ENABLED(did, dmodid)) {
                T_IG(TRACE_GRP_TX, "Tx(Msg): len=%u, dmodid=%s, connid=%u", usr_msg_item->len, vtss_module_names[usr_msg_item->dmodid], usr_msg_item->connid);
            }
        }
        T_NG(TRACE_GRP_TX, "len=%u (first %u bytes shown)", usr_msg_item->len, MIN(96, usr_msg_item->len));
        T_NG_HEX(TRACE_GRP_TX, usr_msg_item->usr_msg, MIN(96, usr_msg_item->len));

        VTSS_ASSERT(mcb != NULL); // Keep Lint happy
        CX_concat_msg_items(&mcb->tx_msg_list, &mcb->tx_msg_list_last, usr_msg_item, usr_msg_item);

        // Wake-up the TX_thread()
        TX_wake_up_msg_thread(MSG_FLAG_TX_MSG);
    }

    MSG_STATE_CRIT_EXIT();
}

/******************************************************************************/
// msg_tx()
// This is the simple form of msg_tx_adv(), and it doesn't support callbacks or
// options.
/******************************************************************************/
void msg_tx(vtss_module_id_t dmodid, u32 did, const void *const msg, size_t len)
{
    // Since callers of msg_tx() must always use VTSS_MALLOC() to allocate the message,
    // and the message module will always free it, and there's no TxDone callback,
    // we don't need to make a copy of the message if looping back.
    msg_tx_adv(NULL, NULL, MSG_TX_OPT_NO_ALLOC_ON_LOOPBACK, dmodid, did, msg, len);
}

/******************************************************************************/
// msg_rx_filter_register()
/******************************************************************************/
mesa_rc msg_rx_filter_register(const msg_rx_filter_t *filter)
{
    mesa_rc rc = MSG_ERROR_PARM;

    // Validate the filter.
    if (!RX_filter_validate(filter)) {
        return rc;
    }

    MSG_CFG_CRIT_ENTER();
    if (!RX_filter_insert(filter)) {
        goto exit_func;
    }

    rc = VTSS_RC_OK;
exit_func:
    MSG_CFG_CRIT_EXIT();
    return rc;
}

/****************************************************************************/
// msg_switch_is_primary()
// Once a MASTER_UP/ICFG_LOADING_PRE event has occurred at the IM_thread(), this
// function will return TRUE until a MASTER_DOWN event occurs at the same
// function. This is regardless of whether the switch is currently a master as
// seen from the SPROUT/Msg protocols.
// The function reports master BEFORE the INIT_CMD_ICFG_LOADING_PRE event is
// pumped out to all modules.
/****************************************************************************/
BOOL msg_switch_is_primary(void)
{
    BOOL result;

    MSG_STATE_CRIT_ENTER();
    result = CX_user_state.master;
    MSG_STATE_CRIT_EXIT();

    return result;
}

/****************************************************************************/
// msg_wait()
/****************************************************************************/
void msg_wait(msg_wait_until_t what, vtss_module_id_t module_id)
{
    T_DG(TRACE_GRP_WAIT, "Enter (module = %s, what = %s)", vtss_module_names[module_id], CX_wait_until_to_str(what));

    switch (what) {
    case MSG_WAIT_UNTIL_INIT_DONE:
        CX_init_done_lock.wait();
        break;

    case MSG_WAIT_UNTIL_ICFG_LOADING_PRE:
        CX_master_up_pre_lock.wait();
        break;

    case MSG_WAIT_UNTIL_ICFG_LOADING_POST:
        CX_master_up_post_lock.wait();
        break;

    case MSG_WAIT_UNTIL_ICFG_LOADED_POST:
        CX_switch_add_post_lock.wait();
        break;

    default:
        T_EG(TRACE_GRP_WAIT, "msg_wait() invoked with invalid argument (%d)", what);
        break;
    }

    T_DG(TRACE_GRP_WAIT, "Exit (module = %s, what = %s)", vtss_module_names[module_id], CX_wait_until_to_str(what));
}

/****************************************************************************/
// msg_switch_exists()
// Once a SWITCH_ADD event has occurred at the IM_thread()
// this function will return TRUE until a SWITCH_DELETE event
// occurs at the same function. This is regardless of whether
// the switch is actually present in the stack or not (it could
// happen that the switch disappears before the SWITCH_DELETE
// event gets to the IM_thread().
/****************************************************************************/
BOOL msg_switch_exists(vtss_isid_t isid)
{
    BOOL result;

    if (!VTSS_ISID_LEGAL(isid)) {
        T_E("Invalid isid: %u", isid);
        return FALSE;
    }

    MSG_STATE_CRIT_ENTER();
    result = CX_user_state.exists[isid];
    MSG_STATE_CRIT_EXIT();

    return result;
}

/****************************************************************************/
// msg_switch_configurable()
/****************************************************************************/
BOOL msg_switch_configurable(vtss_isid_t isid)
{
    BOOL result;

    if (!VTSS_ISID_LEGAL(isid)) {
        T_E("Invalid isid: %u", isid);
        return FALSE;
    }

    MSG_STATE_CRIT_ENTER();
    result = CX_user_state.master && CX_switch_info.info[isid].configurable;
    MSG_STATE_CRIT_EXIT();
    return result;
}

/****************************************************************************/
// msg_existing_switches()
/****************************************************************************/
u32 msg_existing_switches(void)
{
    u32         result = 0;
    vtss_isid_t isid;

    MSG_STATE_CRIT_ENTER();
    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if (CX_user_state.exists[isid]) {
            result |= (1 << isid);
        }
    }
    MSG_STATE_CRIT_EXIT();
    return result;
}

/****************************************************************************/
// msg_configurable_switches()
/****************************************************************************/
u32 msg_configurable_switches(void)
{
    u32         result = 0;
    vtss_isid_t isid;

    MSG_STATE_CRIT_ENTER();
    // Unlike msg_existing_switches(), we need to test whether we're master here, because
    // the "configurable" state is not always FALSE when we're slave.
    if (CX_user_state.master) {
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            if (CX_switch_info.info[isid].configurable) {
                result |= (1 << isid);
            }
        }
    }
    MSG_STATE_CRIT_EXIT();
    return result;
}

/****************************************************************************/
// msg_switch_is_local()
/****************************************************************************/
BOOL msg_switch_is_local(vtss_isid_t isid)
{
    BOOL result;
    MSG_STATE_CRIT_ENTER();
    result = (CX_user_state.master && isid == CX_user_state.master_isid);
    MSG_STATE_CRIT_EXIT();
    return result;
}

/****************************************************************************/
// msg_primary_switch_isid()
// Returns our own isid if we're master, VTSS_ISID_UNKNOWN otherwise.
/****************************************************************************/
vtss_isid_t msg_primary_switch_isid(void)
{
    vtss_isid_t result;
    MSG_STATE_CRIT_ENTER();
    result = CX_user_state.master_isid;
    MSG_STATE_CRIT_EXIT();
    return result;
}

/****************************************************************************/
// msg_version_string_get()
// Returns the version string of a given switch in the stack. The call is only
// valid on the master, and the caller must allocate MSG_MAX_VERSION_STRING_LEN
// bytes for @ver_str prior to the call.
// @ver_str[0] is set to '\0' if an error occurred or no version string was
// received from the slave switch.
// The reason for not just returning a pointer is that the stack may change
// right after the pointer is returned, thus resulting in a non-consistent
// version string.
/****************************************************************************/
void msg_version_string_get(vtss_isid_t isid, char *ver_str)
{
    VTSS_ASSERT(ver_str);
    ver_str[0] = '\0';

    if (!VTSS_ISID_LEGAL(isid)) {
        T_E("Invalid isid: %u", isid);
        return;
    }

    MSG_STATE_CRIT_ENTER();
    if (state.state == MSG_MOD_STATE_PRI) {
        if (mcbs[isid][0].state == MSG_CONN_STATE_PRI_ESTABLISHED) {
            // mcb->u.primary_switch->version_string is guaranteed to be NULL-terminated and less than
            // MSG_MAX_VERSION_STRING_LEN bytes long.
            strcpy(ver_str, mcbs[isid][0].u.primary_switch.switch_info.version_string);
        }
    }
    MSG_STATE_CRIT_EXIT();
}

/****************************************************************************/
// msg_product_name_get()
// Returns the product name of a given switch in the stack. The call is only
// valid on the master, and the caller must allocate MSG_MAX_PRODUCT_NAME_LEN
// bytes for @prod_name prior to the call.
// @prod_name[0] is set to '\0' if an error occurred or no product name was
// received from slave switch.
// The reason for not just returning a pointer is that the stack may change
// right after the pointer is returned, thus resulting in a non-consistent
// version string.
/****************************************************************************/
void msg_product_name_get(vtss_isid_t isid, char *prod_name)
{
    VTSS_ASSERT(prod_name);
    prod_name[0] = '\0';

    if (!VTSS_ISID_LEGAL(isid)) {
        T_E("Invalid isid: %u", isid);
        return;
    }

    MSG_STATE_CRIT_ENTER();
    if (state.state == MSG_MOD_STATE_PRI) {
        if (mcbs[isid][0].state == MSG_CONN_STATE_PRI_ESTABLISHED) {
            // mcb->u.primary_switch->product_name is guaranteed to be NULL-terminated and less than
            // MSG_MAX_PRODUCT_NAME_LEN bytes long.
            strcpy(prod_name, mcbs[isid][0].u.primary_switch.switch_info.product_name);
        }
    }
    MSG_STATE_CRIT_EXIT();
}

/****************************************************************************/
// msg_uptime_get()
// Returns the up-time measured in seconds of a switch.
// This function may be called both as a master and as a slave.
// If calling as a slave, @isid must be VTSS_ISID_LOCAL.
// If calling as a master, @isid may be any legal ISID and VTSS_ISID_LOCAL.
// If calling as a master and @isid is not present, 0 is returned.
/****************************************************************************/
time_t msg_uptime_get(vtss_isid_t isid)
{
    time_t uptime;

    MSG_STATE_CRIT_ENTER();
    // CX_uptime_get_crit_taken() returns an u32.
    // time_t is also an u32 and represents a number of seconds, just as
    // CX_uptime_get_crit_taken() does.
    uptime = CX_uptime_get_crit_taken(isid);
    MSG_STATE_CRIT_EXIT();
    return uptime;
}

/****************************************************************************/
// msg_abstime_get()
// As input it takes a relative time measured in seconds since boot and an
// isid. The isid is used to tell on which switch the relative time was
// recorded on. The returned time is a time_t value that takes into account
// the current switch's SNTP-obtained time.
// The return value can be converted to a string using misc_time2str().
//
// If calling as a slave, @isid must be VTSS_ISID_LOCAL.
// If calling as a master, @isid may be any legal ISID and VTSS_ISID_LOCAL.
// If calling as a master and @isid is not present, 0 is returned.
/****************************************************************************/
time_t msg_abstime_get(vtss_isid_t isid, time_t rel_event_time)
{
    time_t cur_abstime, cur_uptime, abs_boot_time_of_this_switch, result, diff_time;

    MSG_STATE_CRIT_ENTER();

    VTSS_ASSERT(VTSS_ISID_LEGAL(isid) || isid == VTSS_ISID_LOCAL);

    // Get the current absolute time on the local switch, which is the
    // one that we convert all times into.
    cur_abstime = time(NULL);

    // Get the number of seconds that this switch has currently been up since boot.
    cur_uptime = CX_uptime_secs_get();

    // "Luckily", time() is measured in seconds, so that the uptime_secs
    // and time_t are compatible.
    if (cur_abstime < cur_uptime) {
        T_W("Current absolute time (%u) is smaller than the switch's uptime (%u)", (u32)cur_abstime, (u32)cur_uptime);
        cur_abstime = cur_uptime;
    }

    // Compute the absolute time that this switch was booted
    abs_boot_time_of_this_switch = cur_abstime - cur_uptime;

    if (isid == VTSS_ISID_LOCAL) {
        // No need to look up difference between master and slave time.
        result = abs_boot_time_of_this_switch + rel_event_time;
    } else if (state.state == MSG_MOD_STATE_PRI && mcbs[isid][0].state == MSG_CONN_STATE_PRI_ESTABLISHED) {
        // We're master and the connection is established.
        // We need to take special care of the case where the slave's uptime is
        // bigger than that of this switch's.
        if (mcbs[isid][0].u.primary_switch.switch_info.slv_uptime_secs > mcbs[isid][0].u.primary_switch.switch_info.mst_uptime_secs) {
            diff_time = mcbs[isid][0].u.primary_switch.switch_info.slv_uptime_secs - mcbs[isid][0].u.primary_switch.switch_info.mst_uptime_secs;
            // The slave booted diff_time seconds before us.
            // Check if the event happened before this switch was booted
            if (diff_time > rel_event_time) {
                // The event (at rel_event_time) happened before this switch was booted.
                diff_time -= rel_event_time;
                // diff_time now holds the number of seconds that the event happened before
                // this switch was booted. If this is a bigger number than this switch's absolute
                // boot time, we use this switch's absolute boot time as the result. When this
                // happens, it's an indication that this switch hasn't gotten an SNTP time.
                if (diff_time > abs_boot_time_of_this_switch) {
                    result = abs_boot_time_of_this_switch;
                } else {
                    // This switch has probably gotten an SNTP time. The event still
                    // happened before this switch was booted, though.
                    result = abs_boot_time_of_this_switch - diff_time;
                }
            } else {
                // The event happened after the master switch (this switch) was booted.
                result = (rel_event_time - diff_time) + abs_boot_time_of_this_switch;
            }
        } else {
            // The slave switch booted at the same time or after the master (this) switch.
            diff_time = mcbs[isid][0].u.primary_switch.switch_info.mst_uptime_secs - mcbs[isid][0].u.primary_switch.switch_info.slv_uptime_secs;
            // diff_time now holds the number of seconds after the master switch booted that
            // the slave switch booted.
            // abs_boot_time_of_this_switch + diff_time is the absolute boot time of the slave switch.
            // The event happened rel_event_time seconds after the the slave switch booted.
            result = abs_boot_time_of_this_switch + diff_time + rel_event_time;
        }
    } else {
        // We're not master or the slave is not connected.
        result = 0;
    }

    MSG_STATE_CRIT_EXIT();
    return result;
}

/******************************************************************************/
// msg_switch_info_get()
/******************************************************************************/
mesa_rc msg_switch_info_get(vtss_isid_t isid, msg_switch_info_t *info)
{
    mesa_rc result = VTSS_RC_OK;

    if ((!VTSS_ISID_LEGAL(isid) && isid != VTSS_ISID_LOCAL) || info == NULL) {
        return VTSS_RC_ERROR;
    }

    MSG_STATE_CRIT_ENTER();

    if (isid == VTSS_ISID_LOCAL) {
        isid = state.misid;
    }
    if (state.state == MSG_MOD_STATE_PRI && mcbs[isid][0].state == MSG_CONN_STATE_PRI_ESTABLISHED) {
        *info = mcbs[isid][0].u.primary_switch.switch_info;
    } else {
        result = VTSS_RC_ERROR;
    }

    MSG_STATE_CRIT_EXIT();
    return result;
}

/******************************************************************************/
// msg_buf_pool_create()
/******************************************************************************/
void *msg_buf_pool_create(vtss_module_id_t module_id, const char *dscr, u32 buf_cnt, u32 bytes_per_buf)
{
    u32            pool_size, buf_size, alloc_size;
    void           *mem;
    msg_buf_pool_t *pool;
    msg_buf_t      *buf_iter;
    int            i;

    // Avoid Lint Warning 429: Custodial pointer 'mem' (line 5745) has not been freed or returned
    /*lint --e{429} */

    if (module_id >= VTSS_MODULE_ID_NONE || buf_cnt == 0 || bytes_per_buf == 0) {
        T_E("Invalid arg");
        return NULL;
    }

    pool_size  = MSG_ALIGN64(sizeof(msg_buf_pool_t));
    buf_size   = MSG_ALIGN64(sizeof(msg_buf_t)) + MSG_ALIGN64(bytes_per_buf);
    alloc_size = pool_size + buf_cnt * buf_size;

    if ((mem = VTSS_MALLOC(alloc_size)) == NULL) {
        T_E("Can't allocate %u bytes", alloc_size);
        return NULL;
    }

    pool = (msg_buf_pool_t *)MSG_ALIGN64(mem);

    // Create a counting semaphore.
    vtss_sem_init(&pool->sem, buf_cnt);
    pool->magic        = MSG_BUF_POOL_MAGIC;
    pool->dscr         = (char *)dscr;
    pool->buf_cnt_init = buf_cnt;
    pool->buf_cnt_cur  = buf_cnt;
    pool->buf_cnt_min  = buf_cnt;
    pool->buf_size     = bytes_per_buf;
    pool->allocs       = 0;
    pool->module_id    = module_id;
    pool->used         = NULL;

    // Set-up linked lists.
    buf_iter = (msg_buf_t *)((u8 *)pool + pool_size);
    pool->free = buf_iter;
    for (i = 0; i < (int)buf_cnt; i++) {
        buf_iter->pool = pool;
        buf_iter->buf  = (msg_buf_t *)((u8 *)buf_iter + MSG_ALIGN64(sizeof(msg_buf_t)));
        buf_iter->next = (msg_buf_t *)((u8 *)buf_iter->buf + MSG_ALIGN64(bytes_per_buf));
        if (i < (int)(buf_cnt - 1)) {
            buf_iter = buf_iter->next;
        }
    }
    buf_iter->next = NULL;

    if ((u8 *)buf_iter + buf_size != (u8 *)mem + alloc_size) {
        T_E("Bad implementation");
    }

    // Allow this function to be called during INIT_CMD_INIT, so don't
    // use MSG_BUF_CRIT_ENTER/EXIT()() but simply lock and unlock the scheduler.
    vtss_global_lock(__FILE__, __LINE__);
    pool->next = MSG_buf_pool;
    MSG_buf_pool = pool;
    vtss_global_unlock(__FILE__, __LINE__);

    return pool;
}

/******************************************************************************/
// MSG_buf_pool_get()
/******************************************************************************/
static void *MSG_buf_pool_get(void *buf_pool, BOOL wait)
{
    msg_buf_pool_t *pool = (msg_buf_pool_t *)buf_pool;
    msg_buf_t      *buf_ptr;

    if (!pool || pool->magic != MSG_BUF_POOL_MAGIC) {
        T_E("Invalid pool magic");
        return NULL;
    }

    if (wait) {
        vtss_sem_wait(&pool->sem);
    } else if (!vtss_sem_trywait(&pool->sem)) {
        return NULL;
    }

    MSG_BUF_CRIT_ENTER();
    buf_ptr = pool->free;
    VTSS_ASSERT(buf_ptr != NULL);

    if (pool->buf_cnt_cur == 0) {
        T_E("Invalid implementation");
    } else {
        pool->buf_cnt_cur--;
        if (pool->buf_cnt_cur < pool->buf_cnt_min) {
            pool->buf_cnt_min = pool->buf_cnt_cur;
        }
    }
    pool->allocs++;
    pool->free       = buf_ptr->next;
    buf_ptr->next    = pool->used;
    buf_ptr->ref_cnt = 1;
    pool->used       = buf_ptr;

    MSG_BUF_CRIT_EXIT();
    VTSS_ASSERT(buf_ptr->buf != NULL);
    return buf_ptr->buf;
}

/******************************************************************************/
// msg_buf_pool_get()
/******************************************************************************/
void *msg_buf_pool_get(void *buf_pool)
{
    return MSG_buf_pool_get(buf_pool, TRUE);
}

/******************************************************************************/
// msg_buf_pool_try_get()
/******************************************************************************/
void *msg_buf_pool_try_get(void *buf_pool)
{
    return MSG_buf_pool_get(buf_pool, FALSE);
}

/******************************************************************************/
// msg_buf_pool_put()
/******************************************************************************/
u32 msg_buf_pool_put(void *buf)
{
    msg_buf_t      *buf_ptr, *iter, *prev;
    msg_buf_pool_t *pool;
    u32            ref_cnt = 0;

    buf_ptr = (msg_buf_t *)((u8 *)buf - MSG_ALIGN64(sizeof(msg_buf_t)));

    if (!buf_ptr || !buf_ptr->pool || buf_ptr->pool->magic != MSG_BUF_POOL_MAGIC) {
        T_E("Invalid buf");
        return ref_cnt;
    }

    pool = buf_ptr->pool;

    MSG_BUF_CRIT_ENTER();

    if (buf_ptr->ref_cnt == 0) {
        T_E("Invalid implementation");
    } else if ((ref_cnt = --buf_ptr->ref_cnt) == 0) {
        iter = pool->used;
        prev = NULL;
        while (iter && iter != buf_ptr) {
            prev = iter;
            iter = iter->next;
        }

        if (!iter) {
            T_E("No such buffer");
            MSG_BUF_CRIT_EXIT();
            return ref_cnt;
        }

        if (prev == NULL) {
            pool->used = buf_ptr->next;
        } else {
            prev->next = buf_ptr->next;
        }

        if (pool->buf_cnt_cur == pool->buf_cnt_init) {
            T_E("Invalid implementation");
        } else {
            pool->buf_cnt_cur++;
        }

        buf_ptr->next = pool->free;
        pool->free = buf_ptr;
        vtss_sem_post(&pool->sem);
    }

    MSG_BUF_CRIT_EXIT();
    return ref_cnt;
}

/******************************************************************************/
// msg_buf_pool_ref_cnt_set()
/******************************************************************************/
void msg_buf_pool_ref_cnt_set(void *buf, u32 ref_cnt)
{
    msg_buf_t *buf_ptr;

    buf_ptr = (msg_buf_t *)((u8 *)buf - MSG_ALIGN64(sizeof(msg_buf_t)));

    if (!buf_ptr || !buf_ptr->pool || buf_ptr->pool->magic != MSG_BUF_POOL_MAGIC) {
        T_E("Invalid buf");
        return;
    }

    if (ref_cnt == 0) {
        MSG_BUF_CRIT_ENTER();
        buf_ptr->ref_cnt = 1;
        MSG_BUF_CRIT_EXIT();
        (void)msg_buf_pool_put(buf);
    } else {
        MSG_BUF_CRIT_ENTER();
        buf_ptr->ref_cnt = ref_cnt;
        MSG_BUF_CRIT_EXIT();
    }
}

/******************************************************************************/
// msg_max_user_prio()
/******************************************************************************/
u32 msg_max_user_prio(void)
{
    return VTSS_PRIOS - 1;
}

/******************************************************************************/
// msg_stp_port_state_set()
/******************************************************************************/
mesa_rc msg_stp_port_state_set(const mesa_inst_t inst, const mesa_port_no_t port_no, const mesa_stp_state_t stp_state)
{
    return mesa_stp_port_state_set(inst, port_no, stp_state);
}

/******************************************************************************/
/******************************************************************************/
typedef enum {
    MSG_DBG_CMD_STAT_USR_MSG_PRINT = 1,
    MSG_DBG_CMD_STAT_PROTOCOL_PRINT,
    MSG_DBG_CMD_STAT_LAST_RX_CB_PRINT,
    MSG_DBG_CMD_STAT_LAST_IM_CB_PRINT,
    MSG_DBG_CMD_STAT_SHAPER_PRINT,
    MSG_DBG_CMD_STAT_IM_MAX_CB_TIME_PRINT,
    MSG_DBG_CMD_STAT_POOL_PRINT,
    MSG_DBG_CMD_SWITCH_INFO_PRINT,
    MSG_DBG_CMD_SWITCH_INFO_RELOAD,
    MSG_DBG_CMD_STAT_USR_MSG_CLEAR = 10,
    MSG_DBG_CMD_STAT_PROTOCOL_CLEAR,
    MSG_DBG_CMD_STAT_RX_MAX_CB_TIME_PRINT,
    MSG_DBG_CMD_CFG_TRACE_ISID_SET = 20,
    MSG_DBG_CMD_CFG_TRACE_MODID_SET,
    MSG_DBG_CMD_CFG_TIMEOUT,
    MSG_DBG_CMD_CFG_RETRANSMIT,
    MSG_DBG_CMD_CFG_WINSZ,
    MSG_DBG_CMD_CFG_FLASH_CLEAR,
    MSG_DBG_CMD_CFG_HTL,
    MSG_DBG_CMD_TEST_RUN = 40,
    MSG_DBG_CMD_TEST_RENEGOTIATE,
    MSG_DBG_CMD_TEST_MASTER_DOWN_UP,
} msg_dbg_cmd_num_t;

/******************************************************************************/
/******************************************************************************/
typedef struct {
    msg_dbg_cmd_num_t cmd_num;
    const char        *cmd_txt;
    const char        *arg_syntax;
    uint              max_arg_cnt;
    void              (*func)(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms);
} msg_dbg_cmd_t;

/******************************************************************************/
/******************************************************************************/
static msg_dbg_cmd_t msg_dbg_cmds[] = {
    {
        MSG_DBG_CMD_STAT_USR_MSG_PRINT,
        "Print User Message Statistics",
        NULL,
        0,
        DBG_cmd_stat_usr_msg_print
    },
    {
        MSG_DBG_CMD_STAT_PROTOCOL_PRINT,
        "Print Message Protocol Statistics",
        NULL,
        0,
        DBG_cmd_stat_protocol_print
    },
    {
        MSG_DBG_CMD_STAT_LAST_RX_CB_PRINT,
        "Print last callback to Msg Rx",
        NULL,
        0,
        DBG_cmd_stat_last_rx_cb_print
    },
    {
        MSG_DBG_CMD_STAT_LAST_IM_CB_PRINT,
        "Print last callback to init_modules()",
        NULL,
        0,
        DBG_cmd_stat_last_im_cb_print
    },
    {
        MSG_DBG_CMD_STAT_SHAPER_PRINT,
        "Print shaper status",
        NULL,
        0,
        DBG_cmd_stat_shaper_print
    },
    {
        MSG_DBG_CMD_STAT_IM_MAX_CB_TIME_PRINT,
        "Print longest init_modules() callback time",
        "[clear]",
        1,
        DBG_cmd_stat_im_max_cb_time_print
    },
    {
        MSG_DBG_CMD_STAT_POOL_PRINT,
        "Print message pool statistics",
        NULL,
        0,
        DBG_cmd_stat_pool_print
    },
    {
        MSG_DBG_CMD_SWITCH_INFO_PRINT,
        "Print switch info",
        NULL,
        0,
        DBG_cmd_switch_info_print
    },
    {
        MSG_DBG_CMD_SWITCH_INFO_RELOAD,
        "Reload switch info from flash (has an effect on slaves only)",
        NULL,
        0,
        DBG_cmd_switch_info_reload
    },
    {
        MSG_DBG_CMD_STAT_USR_MSG_CLEAR,
        "Clear User Message Statistics",
        NULL,
        0,
        DBG_cmd_stat_usr_msg_clear
    },
    {
        MSG_DBG_CMD_STAT_PROTOCOL_CLEAR,
        "Clear Message Protocol Statistics",
        NULL,
        0,
        DBG_cmd_stat_protocol_clear
    },
    {
        MSG_DBG_CMD_STAT_RX_MAX_CB_TIME_PRINT,
        "Print longest Msg Rx callback time",
        "[clear]",
        1,
        DBG_cmd_stat_rx_max_cb_time_print
    },
    {
        MSG_DBG_CMD_CFG_TRACE_ISID_SET,
        "Configure trace output per ISID - only used when master",
        "[<isid> [<enable> (0 or 1)]] - Use <isid> = 0 to enable or disable all",
        2,
        DBG_cmd_cfg_trace_isid_set
    },
    {
        MSG_DBG_CMD_CFG_TRACE_MODID_SET,
        "Configure trace output per module",
        "[<module_id> [<enable> (0 or 1)]] - Use <module_id> = -1 to enable or disable all",
        2,
        DBG_cmd_cfg_trace_modid_set
    },
    {
        MSG_DBG_CMD_CFG_TIMEOUT,
        "Configure timeouts",
        "[<MSYN timeout> [<MD timeout>]] - all in milliseconds (0 = no change)",
        2,
        DBG_cmd_cfg_timeout
    },
    {
        MSG_DBG_CMD_CFG_RETRANSMIT,
        "Configure retransmits",
        "[<MD retransmits>]",
        1,
        DBG_cmd_cfg_retransmit
    },
    {
        MSG_DBG_CMD_CFG_WINSZ,
        "Configure window sizes",
        "[[<master per slave> [<slave>]] (0 = no change)",
        2,
        DBG_cmd_cfg_winsz
    },
    {
        MSG_DBG_CMD_CFG_FLASH_CLEAR,
        "Erase message module's knowledge about other switches. Takes effect upon next boot.",
        NULL,
        0,
        DBG_cmd_cfg_flash_clear
    },
    {
        MSG_DBG_CMD_TEST_RENEGOTIATE,
        "Re-negotiate a connection. ISID argument is not needed on slave, and optional on master",
        "[<ISID>] - leave out to re-negotiate all (master only)",
        1,
        DBG_cmd_test_renegotiate
    },
    {
        MSG_DBG_CMD_TEST_MASTER_DOWN_UP,
        "Send SWITCH_DEL(<ourselves>), MASTER_DOWN, MASTER_UP, SWITCH_ADD(<ourselves> into init_modules()",
        NULL,
        0,
        DBG_cmd_test_master_down_up
    },
    {
        (msg_dbg_cmd_num_t)0,
        NULL,
        NULL,
        0,
        NULL
    }
};

/******************************************************************************/
// msg_dbg()
/******************************************************************************/
void msg_dbg(msg_dbg_printf_t dbg_printf, u32 parms_cnt, u32 *parms)
{
    int i;
    u32 cmd_num;
    if (parms_cnt == 0) {
        (void)dbg_printf("Usage: debug msg <cmd idx>\n");
        (void)dbg_printf("Most commands show current settings if called without arguments\n\n");
        (void)dbg_printf("Commands:\n");
        i = 0;
        while (msg_dbg_cmds[i].cmd_num != 0) {
            (void)dbg_printf("  %2d: %s\n", msg_dbg_cmds[i].cmd_num, msg_dbg_cmds[i].cmd_txt);
            if (msg_dbg_cmds[i].arg_syntax && msg_dbg_cmds[i].arg_syntax[0]) {
                (void)dbg_printf("      Arguments: %s.\n", msg_dbg_cmds[i].arg_syntax);
            }
            i++;
        }
        return;
    }

    cmd_num = parms[0];

    // Verify that command is known and argument count is correct
    i = 0;
    while (msg_dbg_cmds[i].cmd_num != 0) {
        if (msg_dbg_cmds[i].cmd_num == cmd_num) {
            break;
        }
        i++;
    }
    if (msg_dbg_cmds[i].cmd_num == 0) {
        DBG_cmd_syntax_error(dbg_printf, "Unknown command number: %d", cmd_num);
        return;
    }
    if (parms_cnt - 1 > msg_dbg_cmds[i].max_arg_cnt) {
        DBG_cmd_syntax_error(dbg_printf, "Incorrect number of arguments (%d).\n"
                             "Arguments: %s",
                             parms_cnt - 1,
                             msg_dbg_cmds[i].arg_syntax);
        return;
    }
    if (msg_dbg_cmds[i].func == NULL) {
        (void)dbg_printf("Internal Error: Function for command %u not implemented (yet?)", cmd_num);
        return;
    }

    msg_dbg_cmds[i].func(dbg_printf, parms_cnt - 1, parms + 1);
}

/******************************************************************************/
// msg_init_done()
// Function only meant to be called from main.c when the INIT_CMD_INIT
// event has propagated through all modules (therefore, it's not published
// in msg_api.h).
/******************************************************************************/
void msg_init_done(void)
{
    CX_init_done_lock.lock(false);
}

extern "C" int msg_icli_cmd_register();

/******************************************************************************/
// msg_init()
// Initialize Message Module
/******************************************************************************/
mesa_rc msg_init(vtss_init_data_t *data)
{
    /*lint --e{454,456} ... We leave the Mutex locked */
    if (data->cmd == INIT_CMD_INIT) {
        int              cid;
        vtss_isid_t      isid;
        vtss_module_id_t modid;

        for (modid = 0; modid < VTSS_MODULE_ID_NONE; modid++) {
            msg_shaper[modid].limit = MSG_SHAPER_DEFAULT_LIMIT;
        }

        CX_user_state.master_isid = VTSS_ISID_UNKNOWN;

        MSG_ASSERT(VTSS_MODULE_ID_NONE < 256, "Message module supports at most 255 module IDs");

        // Nothing will work if ISID==0 is not reserved, and VTSS_ISID_START is not 1,
        // since we use the ISIDs in the API calls as the first index into the
        // mcbs[][] array.
        VTSS_ASSERT(VTSS_ISID_START == 1);

        // Also the selected sampling time must be something bigger than the tick rate.
        VTSS_ASSERT(VTSS_OS_MSEC2TICK(MSG_SAMPLE_TIME_MS) > 1);

        pend_rx_list = pend_rx_list_last = pend_tx_done_list = pend_tx_done_list_last = NULL;

        for (isid = 0; isid < VTSS_ISID_CNT + 1; isid++) {
            msg_trace_enabled_per_isid[isid] = TRUE;
        }

        for (modid = 0; modid < VTSS_MODULE_ID_NONE; modid++) {
            msg_trace_enabled_per_modid[modid] = TRUE;
        }

        msg_icli_cmd_register();

        // Per default disable TOPO's trace output, since it sends periodic updates,
        // which are disturbing.
        msg_trace_enabled_per_modid[VTSS_MODULE_ID_TOPO] = FALSE;

        // Initialize global state.
        state.state = MSG_MOD_STATE_SEC; // Boot in slave mode.
        state.sec_next_connid = VTSS_ISID_END;
        state.msg_cfg_msyn_timeout_ms          = MSG_CFG_DEFAULT_MSYN_TIMEOUT_MS;
        state.msg_cfg_md_timeout_ms            = MSG_CFG_DEFAULT_MD_TIMEOUT_MS;
        state.msg_cfg_md_retransmit_limit      = MSG_CFG_DEFAULT_MD_RETRANSMIT_LIMIT;
        state.msg_cfg_sec_winsz                = MSG_CFG_DEFAULT_SEC_WINSZ;
        state.msg_cfg_pri_winsz_per_sec        = MSG_CFG_DEFAULT_PRI_WINSZ_PER_SEC;
        state.msg_cfg_htl_limit                = MSG_CFG_DEFAULT_HTL_LIMIT;

        // Reset statistics
        memset(&state.usr_stat[0], 0, sizeof(state.usr_stat));
        memset(&state.pdu_stat[0], 0, sizeof(state.pdu_stat));
        memset(&state.relay_stat[0], 0, sizeof(state.relay_stat));

        // Initialize MCBs.
        for (isid = 0; isid < VTSS_ISID_CNT + 1; isid++) {
            for (cid = 0; cid < MSG_CFG_CONN_CNT; cid++) {
                msg_mcb_t *mcb = &mcbs[isid][cid];

                if (isid == MSG_SLV_ISID_IDX) {
                    mcb->state = MSG_CONN_STATE_SEC_NO_PRI;
                } else {
                    CX_set_state_mst_no_slv(mcb);
                }

                mcb->connid              = isid;         // mcbs[0] is used when slave and holds the connection ID towards the master. mcbs[1; 16]'s connid is only used to filter debug trace messages in TX_mack().
                mcb->next_available_sseq = isid * 10000; // Initialized here and never more. Pick different numbers to ease debugging
                mcb->tx_msg_list         = NULL;
                mcb->tx_msg_list_last    = NULL;
                mcb->rx_msg_list         = NULL;
                // Remaining fields are initialized as the states are changed.
            }
        }

        // Create semaphore. Initially locked, since we need to load our configuration
        // before we allow others to call us. The conf is loaded in the beginning of
        // TX_thread(), after which the semaphores are released.
        critd_init_legacy(&crit_msg_state, "Msg state", VTSS_MODULE_ID_MSG, CRITD_TYPE_MUTEX);

        // Mutex protecting our counters
        critd_init_legacy(&crit_msg_counters, "Msg counters", VTSS_MODULE_ID_MSG, CRITD_TYPE_MUTEX);

        // Critical region protecting the message subscription filter list.
        critd_init_legacy(&crit_msg_cfg, "Msg config", VTSS_MODULE_ID_MSG, CRITD_TYPE_MUTEX);

        // Critical region protecting the Rx and Tx Done callback lists.
        critd_init_legacy(&crit_msg_pend_list, "Msg pending", VTSS_MODULE_ID_MSG, CRITD_TYPE_MUTEX);

        // Critical region protecting IM_fifo.
        critd_init_legacy(&crit_msg_im_fifo, "Msg Init Mods FIFO", VTSS_MODULE_ID_MSG, CRITD_TYPE_MUTEX);

        // Mutex protecting message buffers
        critd_init(&crit_msg_buf, "Msg buffers", VTSS_MODULE_ID_MSG, CRITD_TYPE_MUTEX);

        // Initialize FIFO for transporting init_modules() call messages from any thread to the IM_thread.
        IM_fifo_cnt = IM_fifo_rd_idx = IM_fifo_wr_idx = 0;

        // Create a flag that can wake up the IM_thread.
        vtss_flag_init(&IM_flag);

        // Create a flag that can wake up the TX_thread.
        vtss_flag_init(&TX_msg_flag);

        // Create a flag that can wake up the RX_thread.
        vtss_flag_init(&RX_msg_flag);

        // Create Init Modules Thread
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           IM_thread,
                           0,
                           "Init Modules",
                           nullptr,
                           0,
                           &IM_thread_handle,
                           &IM_thread_state);

        // Create TX thread.
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           TX_thread,
                           0,
                           "Message TX",
                           nullptr,
                           0,
                           &TX_msg_thread_handle,
                           &TX_msg_thread_state);

        // Create RX thread. Resumed from TX thread.
        vtss_thread_create(VTSS_THREAD_PRIO_ABOVE_NORMAL,
                           RX_thread,
                           0,
                           "Message RX",
                           nullptr,
                           0,
                           &RX_msg_thread_handle,
                           &RX_msg_thread_state);
    }

    return VTSS_RC_OK;
}

