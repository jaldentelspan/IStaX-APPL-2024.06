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

// Introduction
// ------------
// critd implements a mutex and semaphore wrapper, which adds checks and other
// debug facilities.
//
// When using critd it is assumed that:
// - A mutex is always released by the same thread that acquired it.
// - A semaphore may be acquired by one thread and signalled by another.

#ifndef _CRITD_API_H_
#define _CRITD_API_H_

#include "vtss_os_wrapper.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CRITD_NAME_LEN 24

#define CRITD_LOCK_ATTEMPT_SIZE     16
#define CRITD_MAX_LOCK_TIME_DEFAULT 60

#define VTSS_CRIT_SCOPE_CLASS__(CRIT, NAME)                  \
    struct NAME {                                            \
        NAME(int line) {critd_enter(&CRIT, __FILE__, line);} \
        ~NAME() {       critd_exit( &CRIT, __FILE__, 0);}    \
    }

#define VTSS_CRIT_SCOPE_CLASS_EXTERN(CRIT, NAME) \
    extern critd_t CRIT;                         \
    VTSS_CRIT_SCOPE_CLASS__(CRIT, NAME)

#define VTSS_CRIT_SCOPE_CLASS(CRIT, NAME) \
    static critd_t CRIT;                  \
    VTSS_CRIT_SCOPE_CLASS__(CRIT, NAME)

typedef enum {
  /**< Mutex
   * Used to protect critical sections.
   * Unlock must occur from the same thread that locks it,
   * with one single exception: Since the mutex is created
   * locked (typically you would create the mutex in
   * INIT_CMD_INIT), then a critd_exit() call is needed to
   * unlock it the first time. This call may be placed in any
   * thread context, but will typically be called after you've
   * loaded the data that the mutex protects with valid data
   * - for instance loaded from flash.
   * This is supported by this Mutex wrapper.
   *
   * Unlike semaphores, mutexes support priority inheritance,
   * which avoids the priority inversion problem. So this
   * is the normal critd type you will need.
   *
   * The mutex cannot be entered recursively from the same thread.
   */
  CRITD_TYPE_MUTEX,

  /**< Semaphore
   * Use this type of critd to protect a resource, which by
   * default is available.
   * Locking may occur in one thread and unlocking in another.
   *
   * The semaphore cannot be entered recursively from the same thread.
   * The maximum count is 1.
   */
  CRITD_TYPE_SEMAPHORE,

  /**< Recursive mutex
   * Like a mutex, but it allows the same thread to lock the mutex recursive.
   * Locks and unlocks must be balanced.
   */
  CRITD_TYPE_MUTEX_RECURSIVE,
} critd_type_t;

// Create structure with semaphore and holder information
typedef struct _critd_t {
    uint         cookie;  // Initialized to CRITD_T_COOKIE
    critd_type_t type;
    vtss_flag_t  flag;
    union {
        vtss_mutex_t mutex;
        vtss_sem_t   semaphore;
        vtss_recursive_mutex_t rmutex;
    } m;
    BOOL         init_done;
    bool         leaf;

    // Name (used in trace and assertion output)
    char         name[CRITD_NAME_LEN + 1];

    vtss_module_id_t module_id;

    // Max allowed lock time in seconds. -1 => Infinite lock allowed
    int          max_lock_time;

    // Log last lock attempts
    ulong        lock_attempt_cnt;
    ulong        lock_attempt_thread_id[CRITD_LOCK_ATTEMPT_SIZE];
    int          lock_attempt_line[CRITD_LOCK_ATTEMPT_SIZE];
    const char*  lock_attempt_file[CRITD_LOCK_ATTEMPT_SIZE];
    BOOL         lock_pending[CRITD_LOCK_ATTEMPT_SIZE];

    // Thread ID of current locker. Set to 0 on unlock
    ulong        current_lock_thread_id;

    // Thread ID, function and line of last lock/unlock
    // Thread ID = 0 => Unknown thread ID
    ulong        lock_thread_id;
    const char   *lock_file;
    int          lock_line;
    ulong        lock_time;
    u32          lock_cnt;

    ulong        unlock_thread_id;
    const char   *unlock_file;
    int          unlock_line;

    struct _critd_t *nxt;

    // Surveillance information
    ulong last_lock_cnt;
    uint  lock_poll_cnt;
    BOOL  cookie_illegal;

    // Max + total actual lock time
    vtss_tick_count_t lock_tick_cnt;       // Tick count when the mutex was last taken.
    vtss_tick_count_t total_lock_tick_cnt; // Total time it has been taken.
    vtss_tick_count_t max_lock_tick_cnt;   // Maximum time it has been taken.
    ulong            max_lock_thread_id;
    const char *     max_lock_file;
    int              max_lock_line;
} critd_t;

// Initialize critd_t
// Mutexes are created released
// Semaphores are initialized with 1 token.
// If it's a leaf, no other mutexes must be taken once this is acquired.
// See also critd_init_legacy()
void critd_init(
    critd_t*               const crit_p,
    const char*            const name,
    const vtss_module_id_t module_id,
    const critd_type_t     type,
    const bool             leaf = false);

// Initialize critd_t
// Mutexes are created taken
// Semaphores are initialized without tokens.
// If it's a leaf, no other mutexes must be taken once this is acquired.
// See also critd_init()
void critd_init_legacy(
    critd_t*               const crit_p,
    const char*            const name,
    const vtss_module_id_t module_id,
    const critd_type_t     type,
    const bool             leaf = false);

// Delete a critd mutex from the critd-monitor.
void critd_delete(critd_t *const crit_p);

// Enter critical region (i.e. lock mutex, wait for a token on a semaphore).
// The dry_run flag should only be used to implement conditional variables
void critd_enter(
    critd_t*    const crit_p,
    const char* const file,
    const int   line,
    bool        dry_run = false);

// Exit critical region (i.e. unlock mutex, increase token count on semaphore).
// The dry_run flag should only be used to implement conditional variables
void critd_exit(
    critd_t*   const crit_p,
    const char* const file,
    const int   line,
    bool        dry_run = false);

// Assert semaphore to be locked/unlocked
void critd_assert_locked(
    critd_t*   const crit_p,
    const char* const file,
    const int   line
    );

BOOL critd_is_locked(critd_t *const crit_p);

mesa_rc critd_module_init(vtss_init_data_t *data);

// Debug functions
// ---------------
typedef int (*critd_dbg_printf_t)(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

void critd_dbg_list(
    const critd_dbg_printf_t dbg_printf,
    const vtss_module_id_t   module_id,
    const BOOL               detailed,
    const BOOL               header,
          critd_t            *single_critd_p);

#ifdef VTSS_SW_OPTION_ICLI
void critd_dbg_list_icli(
    const u32               session_id,
    const vtss_module_id_t  module_id,
    const BOOL              detailed,
    const BOOL              header,
    const critd_t           *single_critd_p
);
#endif /* VTSS_SW_OPTION_ICLI */

#ifdef VTSS_SW_OPTION_ICLI
void critd_dbg_max_lock_icli(
    const u32               session_id,
    const vtss_module_id_t  module_id,
    const BOOL              header,
    const BOOL              clear
);
#endif /* VTSS_SW_OPTION_ICLI */

#ifdef __cplusplus
}
#endif
#endif // _CRITD_API_H_


/*
Deadlock debugging

The mutex/semaphore-wrapper on WebStaX is called critd. Whenever a developer needs a mutex or a
semaphore, it is strongly recommended to use the critd wrapper, which not only offers automatic
trace capabilities (lock/unlock messages), but also detects (possible) deadlocks.

The critd module contains its own thread, which monitors the state of all mutexes/semaphores
created with the critd wrapper. If a given mutex/semaphore has been locked for more than 60
seconds (by default), the critd thread prints a list of all currently locked mutexes/semaphores.
This write-up attempts to explain how this info is to be interpreted and utilized to debug the
deadlock.

The following is an example of a deadlock that could have appeared on WebStaX, and it only serves
the purpose of offering an example; any resemblance with existing module names is unintended.

Ports             port.cb_crit             M Locked   0x0000001b 00:00:03 9/port.c#751, 9/port.c#767, lock_attempt_cnt=0x1e>

  [0]=28/port.c#2884  [1]=28/port.c#2884  [2]=28/port.c#2829  [3]=28/port.c#2884  >

  [4]=28/port.c#2829  [5]=28/port.c#2884  [6]=28/port.c#2884  [7]=28/port.c#2829  >

  [8]=28/port.c#2884  [9]=28/port.c#2937  [a]=9/port.c#751  [b]=65/port.c#2829* >

  [c]=32/port.c#2884* [d]=33/port.c#2884* [e]=9/port.c#751  [f]=28/port.c#2884  >

The first line contains an M, which indicates that this is a mutex (as opposed to an S, which would
indicate a semaphore). The first line says: port.cb_crit is currently locked by thread 9, port.c
line 751. The previous time it was unlocked was on thread 9, port.c line 767. The first line also
says: It has been locked 0x1b times in total (including the one that is locking it now), and it
has been attempted locked 0x1e times. This means that three threads are currently waiting for the
mutex (0x1e - 0x1b = 3).

The threads waiting to get this mutex are marked with a *.

In this case that is:

Thread 65 at port.c:2829

Thread 32 at port.c:2884

Thread 33 at port.c:2884

The order that waiters are actually executing both depends on their priority and how many
time-slicing credits the waiter has left when waiting for a mutex.

The next step would be to figure out what thread 9 is currently doing. If you have a look at the
thread information with backtrace that comes after the deadlock, you'll see that thread 9 is
Port control, and the backtrace info is:

  9 Sleep       7       7 Port Control                 N/A        N/A 0x830da620 16384  4952>

#0  0x807cb5e0>

#1  0x807cdc48>

#2  0x8040460c>

#3  0x8044ac08>

#4  0x804b3c3c>

#5  0x804b3fcc>

#6  0x807ce134>

#7  0x80404964>

#8  0x8070fa68>

#9  0x807ce134>

#10 0x804045d0>

#11 0x804b0000>

#12 0x804bc02c>

#13 0x804be3a8>

#14 0x807c9250>

#15 0x807c9224>

The #0 through #15 are return addresses in the thread's call stack. #0 is the outermost return
address (closest to the current execution point), and #15 is the innermost return address
(farthest away from the current execution point).

Typically, the upper two and the lower two are somewhere in eCos (#14 and #15 = thread start and
#0 and #1 = wait for some mutex). Also, #2 and #3 are typically within critd itself, so a good
start point would be the address at #4 (0x804b3c3c).

Open your favorite debugger (DDD, GDB, whatever), load SMBStaX.elf (using file <path_to_ELF_file>),
type list *0x804b3c3c and you get to the return point in the source code. This provided, that the
.elf file that you load is identical to the .elf file that was used for the running image. And
this last point is one of the reasons that it's critical that the bug reporter tells you which
exact version of the software he is using.

Hope this sheds a little light on the critd deadlock trace.
*/

