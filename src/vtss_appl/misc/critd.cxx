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

#include "critd.h"
#include "critd_api.h"
#include "crashhandler.hxx"

#ifdef VTSS_SW_OPTION_SYSUTIL
#include "sysutil_api.h"
#endif

#ifdef VTSS_SW_OPTION_DAYLIGHT_SAVING
#include "daylight_saving_api.h"
#endif

#ifdef VTSS_SW_OPTION_ICLI
#include "icli_api.h"
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_CRITD

/* Trace registration */
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "critd", "Critd module"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#undef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#undef MIN
#define MIN(a,b) ((a) > (b) ? (b) : (a))

#define CRITD_ASSERT(expr, fmt, ...) { \
    if (!(expr)) { \
        T_EXPLICIT(VTSS_TRACE_MODULE_ID, VTSS_TRACE_GRP_DEFAULT, VTSS_TRACE_LVL_ERROR, \
                   file, line, "ASSERTION FAILED"); \
        T_EXPLICIT(VTSS_TRACE_MODULE_ID, VTSS_TRACE_GRP_DEFAULT, VTSS_TRACE_LVL_ERROR, \
                   file, line, fmt, ##__VA_ARGS__); \
        VTSS_ASSERT(expr); \
    } \
}

#define CRITD_THREAD_ID_NONE 0
#define CRITD_T_COOKIE 0x0BADBABE

static vtss_handle_t critd_thread_handle;
static vtss_thread_t critd_thread_block;

// Deadlock surveillance
// Global variable is easier to inspect from gdb
static BOOL critd_deadlock = 0;

/******************************************************************************/
// CritdInternalLock
/******************************************************************************/
struct CritdInternalLock {
    CritdInternalLock()
    {
        // Do not use mutex_init as it will use the trace model, and will not
        // work when called before the trace module is initialized. We want to
        // allow critd to be created in constructors that run before the main
        // function.
        int res;
        pthread_mutexattr_t attr;

        res = pthread_mutexattr_init(&attr);
        VTSS_ASSERT(res == 0);

        // Enable PTHREAD_PRIO_INHERIT
        res = pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
        VTSS_ASSERT(res == 0);

        res = pthread_mutex_init(&m, &attr);
        VTSS_ASSERT(res == 0);
    }

    void lock()
    {
        int res = pthread_mutex_lock(&m);
        VTSS_ASSERT(res == 0);
    }

    void unlock()
    {
        int res = pthread_mutex_unlock(&m);
        VTSS_ASSERT(res == 0);
    }

    pthread_mutex_t m;
};

struct CritdTblLock;

/******************************************************************************/
// CritdInternalTblLock
// We need one non-critd-based mutex for updating the table of critds a given
// module holds.
/******************************************************************************/
static struct CritdInternalTblLock : public CritdInternalLock {
    friend struct CritdTblLock;

private:
    // Linked list of critds for each module
    critd_t *critd_tbl[VTSS_MODULE_ID_NONE];
} critd_internal_tbl_lock;

/******************************************************************************/
// CritdTblLock
/******************************************************************************/
struct CritdTblLock {
    CritdTblLock(const CritdTblLock &rhs) = delete;

    CritdTblLock()
    {
        critd_internal_tbl_lock.lock();
    }

    critd_t *critd_tbl(uint32_t module_id)
    {
        return critd_internal_tbl_lock.critd_tbl[module_id];
    }

    void critd_tbl(uint32_t module_id, critd_t *p)
    {
        critd_internal_tbl_lock.critd_tbl[module_id] = p;
    }

    ~CritdTblLock()
    {
        critd_internal_tbl_lock.unlock();
    }
};

// And we need another mutex when updating attempts to lock a critd.
// We cannot use the same mutex as when updating the critd table, because that
// could result in deadlocks under rare circumstances.
static struct CritdInternalUpdateLock : public CritdInternalLock {
} critd_internal_update_lock;

/******************************************************************************/
// CritUpdateLock
/******************************************************************************/
struct CritdUpdateLock {
    CritdUpdateLock()
    {
        critd_internal_update_lock.lock();
    }

    ~CritdUpdateLock()
    {
        critd_internal_update_lock.unlock();
    }
};

/******************************************************************************/
// critd_peek()
// Current number of tokens
// The function returns 0 if the mutex or semaphore is taken, > 0 otherwise
/******************************************************************************/
static int critd_peek(critd_t *const crit_p)
{
    int res;

    // If legacy-initialized and we've not yet had the initial critd_exit(),
    // then "pretend" that we're locked.
    if (vtss_flag_peek(&crit_p->flag) == 0) {
        return 0;
    }

    switch (crit_p->type) {
    case CRITD_TYPE_MUTEX:
        // Check to see if we can lock it (in the lack of a peek function).
        if (vtss_mutex_trylock(&crit_p->m.mutex)) {
            // The mutex was not taken, but now it is. Undo that.
            vtss_mutex_unlock(&crit_p->m.mutex);
            return 1;
        }

        // The mutex is already taken.
        return 0;

    case CRITD_TYPE_MUTEX_RECURSIVE:
        // We should only be looking at 'lock_cnt' when the mutex is expected
        // to be locked. So we must try to lock it, and if we succeed, we
        // must look at 'lock_cnt' to see if it was locked before we took
        // the lock.
        res = 0; // Assume locked
        if (vtss_recursive_mutex_trylock(&crit_p->m.rmutex)) {
            if (crit_p->m.rmutex.lock_cnt <= 1) {
                res = 1; // Unlocked
            }

            // We just took the mutex, and now we need to release it again,
            // otherwise it will be unbalanced.
            (void)vtss_recursive_mutex_unlock(&crit_p->m.rmutex);
        }

        return res;

    case CRITD_TYPE_SEMAPHORE:
        return (int)vtss_sem_peek(&crit_p->m.semaphore);

    default:
        VTSS_ASSERT(0);
        return 0;
    }
}

/******************************************************************************/
// critd_is_locked()
// Check whether critical region is locked
/******************************************************************************/
BOOL critd_is_locked(critd_t *const crit_p)
{
    return critd_peek(crit_p) == 0;
}

/******************************************************************************/
// critd_dbg_list_()
/******************************************************************************/
static void critd_dbg_list_(CritdTblLock &locked,
                            const critd_dbg_printf_t dbg_printf,
                            const vtss_module_id_t module_id,
                            const BOOL detailed,
                            const BOOL header,
                            critd_t *single_critd_p)
{
    int i;
    const uint MODULE_NAME_WID = 21;
    char module_name_format[10];
    char critd_name_format[10];
    vtss_module_id_t mid_start = 0;
    vtss_module_id_t mid_end = VTSS_MODULE_ID_NONE - 1;
    vtss_module_id_t mid;
    int crit_cnt = 0;
    BOOL first = header;

    /* Work-around for problem with printf("%*s", ...) */
    sprintf(module_name_format, "%%-%ds", MODULE_NAME_WID);
    sprintf(critd_name_format,  "%%-%ds", CRITD_NAME_LEN);

    if (module_id != VTSS_MODULE_ID_NONE) {
        mid_start = module_id;
        mid_end   = module_id;
    }

    for (mid = mid_start; mid <= mid_end; mid++) {
        critd_t *critd_p;
        if (single_critd_p) {
            critd_p = single_critd_p;
        } else {
            critd_p = locked.critd_tbl(mid);
        }

        while (critd_p) {
            if (critd_p->cookie != CRITD_T_COOKIE) {
                T_E("Cookie=0x%08x, expected 0x%08x", critd_p->cookie, CRITD_T_COOKIE);
            }

            crit_cnt++;
            if (first) {
                if (!detailed) {
                    first = 0;
                }

                dbg_printf(module_name_format, "Module");
                dbg_printf(" ");
                dbg_printf(critd_name_format, "Critd Name");
                dbg_printf(" ");
                dbg_printf("T");
                dbg_printf(" ");
                dbg_printf("L");
                dbg_printf(" ");
                dbg_printf("State   "); // "Unlocked" or "Locked"
                dbg_printf(" ");
                dbg_printf("Lock Cnt  ");
                dbg_printf(" ");
                dbg_printf("LockTime ");
                dbg_printf("Latest Lock, Latest Unlock\n");
                for (i = 0; i < MODULE_NAME_WID; i++) {
                    dbg_printf("-");
                }

                dbg_printf(" ");
                for (i = 0; i < CRITD_NAME_LEN; i++) {
                    dbg_printf("-");
                }

                dbg_printf(" ");
                dbg_printf("- -------- ---------- -------- ------------------------------\n");
            }

            dbg_printf(module_name_format, vtss_module_names[mid]);
            dbg_printf(" ");
            dbg_printf(critd_name_format, critd_p->name);
            dbg_printf(" ");
            dbg_printf("%c", critd_p->type == CRITD_TYPE_MUTEX ? 'M' : critd_p->type == CRITD_TYPE_MUTEX_RECURSIVE ? 'R' : 'S');
            dbg_printf(" ");
            dbg_printf("%c", critd_p->leaf ? 'Y' : 'N');
            dbg_printf(" ");

            if (critd_p->current_lock_thread_id) {
                dbg_printf("Locked  ");
            } else if (vtss_flag_peek(&critd_p->flag) == 0) {
                dbg_printf("Unexited");
            } else {
                dbg_printf("Unlocked");
            }

            dbg_printf(" ");

            // Lock cnt
            if (detailed) {
                // Print it in hex in order to be able to compare with lock_attempt_cnt
                char buf[20];
                sprintf(buf, "0x%x", critd_p->lock_cnt);
                dbg_printf("%10s ", buf);
            } else {
                // Print it in decimal, which makes more sense when showing all critds in a list
                dbg_printf("%10u ", critd_p->lock_cnt);
            }

            {
                // Print lock time
                struct tm *timeinfo_p;
                struct tm  timeinfo;
                time_t lock_time;

                lock_time = critd_p->lock_time;
#ifdef VTSS_SW_OPTION_SYSUTIL
                lock_time += (system_get_tz_off() * 60); /* Adjust for TZ minutes => seconds */
#endif
#ifdef VTSS_SW_OPTION_DAYLIGHT_SAVING
                lock_time += (time_dst_get_offset() * 60); /* Correct for DST */
#endif
                timeinfo_p = localtime_r(&lock_time, &timeinfo);

                dbg_printf("%02d:%02d:%02d ", timeinfo_p->tm_hour, timeinfo_p->tm_min, timeinfo_p->tm_sec);
            }
            {
                // Protect against concurrent update of lock information
                char file_lock[100] = "";
                char file_unlock[100] = "";
                strncpy(file_lock,   critd_p->lock_file   ? misc_filename(critd_p->lock_file)   : "",   98);
                strncpy(file_unlock, critd_p->unlock_file ? misc_filename(critd_p->unlock_file) : "", 98);
                file_lock[99]   = 0;
                file_unlock[99] = 0;

                dbg_printf(VPRIlu "/%s#%d, " VPRIlu "/%s#%d",
                           critd_p->lock_thread_id,
                           file_lock,
                           critd_p->lock_line,
                           critd_p->unlock_thread_id,
                           file_unlock,
                           critd_p->unlock_line);
            }

            if (detailed) {
                int i;
                char file_lock[100] = "";
                int j = 0;
                dbg_printf(", lock_attempt_cnt=0x" VPRIlx, critd_p->lock_attempt_cnt);
                for (i = 0; i < CRITD_LOCK_ATTEMPT_SIZE; i++) {
                    if (j++ % 4 == 0) {
                        dbg_printf("\n  ");
                    }

                    strncpy(file_lock,   critd_p->lock_attempt_file[i] ? misc_filename(critd_p->lock_attempt_file[i])   : "",   98);
                    dbg_printf("[%x]=" VPRIlu "/%s#%d%s ",
                               i,
                               critd_p->lock_attempt_thread_id[i],
                               file_lock,
                               critd_p->lock_attempt_line[i],
                               critd_p->lock_pending[i] ? "*" : " ");
                }
            }

            if (single_critd_p) {
                critd_p = NULL;
            } else {
                dbg_printf("\n");
                if (detailed) {
                    dbg_printf("\n");
                }

                critd_p = critd_p->nxt;
            }
        }
    }

    if (module_id != VTSS_MODULE_ID_NONE && crit_cnt == 0) {
    } else if (detailed && header) {
        dbg_printf("Note that the logging of lock attempts could be incorrect, since it is not protected by any critical region.\n");
        dbg_printf("However in most cases the information will likely be correct.\n");
        dbg_printf("\n");
        dbg_printf("The last logged lock attempt is stored in entry [lock_attempt_cnt %% %d].\n",
                   CRITD_LOCK_ATTEMPT_SIZE);
    }
}

/******************************************************************************/
// critd_trace()
/******************************************************************************/
static void critd_trace(critd_t *const crit_p, const char *const file, const int line)
{
    T_EXPLICIT(VTSS_TRACE_MODULE_ID, VTSS_TRACE_GRP_DEFAULT, VTSS_TRACE_LVL_ERROR, file, line,
               "Current state for critical region \"%s\":\n"
               "Locked=%d\n"
               "Latest lock: " VPRIlu "/%s#%d\n"
               "Latest unlock: " VPRIlu "/%s#%d",
               crit_p->name,
               critd_is_locked(crit_p),
               crit_p->lock_thread_id,
               crit_p->lock_file ? misc_filename(crit_p->lock_file) : crit_p->lock_file,
               crit_p->lock_line,
               crit_p->unlock_thread_id,
               crit_p->unlock_file ? misc_filename(crit_p->unlock_file) : crit_p->unlock_file,
               crit_p->unlock_line);
}

static char deadlock_str[10000];

/******************************************************************************/
// critd_deadlock_print()
// Function used to have critd_dbg_list sprint output into deadlock_str buffer
/******************************************************************************/
static int critd_deadlock_print(const char *fmt, ...)
{
    va_list args;
    int     rv;

    va_start(args, fmt);
    rv = vsprintf(deadlock_str + strlen(deadlock_str), fmt, args);
    va_end(args);

    return rv;
}

/******************************************************************************/
// critd_thread()
/******************************************************************************/
static void critd_thread(vtss_addrword_t data)
{
    const uint       POLL_PERIOD = 5;
    vtss_module_id_t mid;
    BOOL             deadlock_found = 0;
    T_D("Enter");
    VTSS_OS_MSLEEP(10000); // Let everything start before starting surveillance

    // Mutex surveillance
    while (1) {
        VTSS_OS_MSLEEP(POLL_PERIOD * 1000);

        for (mid = 0; mid < VTSS_MODULE_ID_NONE; mid++) {
            CritdTblLock locked;
            critd_t *critd_p = locked.critd_tbl(mid);
            while (critd_p) {
                // Check cookie
                if (critd_p->cookie != CRITD_T_COOKIE &&
                    critd_p->cookie_illegal != 1) {
                    T_E("%s: Cookie=0x%08x, expected 0x%08x",
                        vtss_module_names[mid], critd_p->cookie, CRITD_T_COOKIE);
                    critd_p->cookie_illegal = 1;
                }

                if (!critd_deadlock) {
                    // Check for deadlock
                    if (critd_p->current_lock_thread_id && critd_p->max_lock_time != -1) {
                        if (critd_p->lock_cnt == critd_p->last_lock_cnt) {
                            // Same lock as last poll!
                            critd_p->lock_poll_cnt++;
                        } else {
                            // Store lock count, so we can see if same lock persists
                            critd_p->last_lock_cnt = critd_p->lock_cnt;
                            critd_p->lock_poll_cnt      = 0;
                        }
                    } else {
                        critd_p->lock_poll_cnt  = 0;
                    }

                    if (critd_p->lock_poll_cnt > (critd_p->max_lock_time / POLL_PERIOD)) {
                        // Mutex has been locked for too long
                        critd_deadlock = 1;
                        deadlock_found = 1;

                        memset(deadlock_str, 0, sizeof(deadlock_str));
                        critd_dbg_list_(locked, &critd_deadlock_print, mid, 1, 0, critd_p);
                        printf("Error: Mutex deadlock:\n%s\n", deadlock_str);
                    }
                }

                critd_p = critd_p->nxt;
            }
        }

        if (deadlock_found) {
            // Write the state of all locked mutexes to syslog and
            // suspend any further surveillance
            printf("All locked mutexes listed below.\n");
            crashfile_printf("All locked mutexes listed below.\n");
            for (mid = 0; mid < VTSS_MODULE_ID_NONE; mid++) {
                CritdTblLock locked;
                critd_t *critd_p = locked.critd_tbl(mid);
                while (critd_p) {
                    if (critd_p->current_lock_thread_id || vtss_flag_peek(&critd_p->flag) == 0 /* never unlocked the first time */) {
                        memset(deadlock_str, 0, sizeof(deadlock_str));
                        critd_dbg_list_(locked, &critd_deadlock_print, mid, 1, 0, critd_p);
                        // The following call to printf() used to be a call to
                        // T_E(), but if going through the trace module, we
                        // cannot guarantee that it comes out to the console,
                        // because it uses the syslog, which takes mutexes that
                        // may be locked (led module's and syslog's own). The
                        // drawback of not calling T_E() is that we don't get it
                        // saved to flash.
                        printf("\n%s\n", deadlock_str);
                        crashfile_printf("\n%s\n", deadlock_str);
                    }

                    critd_p = critd_p->nxt;
                }
            }

            crashfile_close();

            // For Linux we simply stop all threads, and make the assert print the thread details and reboot the switch.
            VTSS_ASSERT(0);

            // Unreachable
        }
    }
}

// Per-thread variable that holds a pointer to a critd marked as a leaf. When
// non-NULL, no other critd must be taken.
static __thread critd_t *leaf_crit_p;
static volatile bool leaf_detection_abuse_disabled;

/******************************************************************************/
// critd_init_()
/******************************************************************************/
static void critd_init_(critd_t *const crit_p, const char *const name,
                        const vtss_module_id_t module_id,
                        const critd_type_t type, const bool leaf,
                        bool create_locked)
{
    CritdTblLock locked;
    memset(crit_p, 0, sizeof(critd_t));

    crit_p->max_lock_time = CRITD_MAX_LOCK_TIME_DEFAULT;
    crit_p->cookie        = CRITD_T_COOKIE;
    crit_p->type          = type;
    crit_p->leaf          = leaf;

    // From the application point of view, the critd is created locked
    // to allow the application to do some initializations of the data
    // it protects before opening up for other modules to access the
    // data.
    // In reality, this is not possible with mutexes, so a flag is used
    // to indicate whether the critd has been exited the first time.
    // To make the implementation alike for both semaphores and mutexes,
    // the semaphore is created unlocked, and the flag is used to stop
    // access until the initial critd_exit() call.
    vtss_flag_init(&crit_p->flag);

    switch (type) {
    case CRITD_TYPE_MUTEX:
        // Always created unlocked.
        vtss_mutex_init(&crit_p->m.mutex);
        break;

    case CRITD_TYPE_SEMAPHORE:
        // Create it unlocked. The @flag holds back accesses.
        vtss_sem_init(&crit_p->m.semaphore, 1);
        break;

    case CRITD_TYPE_MUTEX_RECURSIVE:
        // Created unlocked
        vtss_recursive_mutex_init(&crit_p->m.rmutex);
        break;

    default:
        VTSS_ASSERT(0);
    }

    strncpy(crit_p->name, name, MIN(CRITD_NAME_LEN, strlen(name)));
    crit_p->module_id                = module_id;
    crit_p->current_lock_thread_id   = CRITD_THREAD_ID_NONE;
    crit_p->nxt                      = NULL;

    // Insert into front of the linked list
    if (locked.critd_tbl(module_id)) {
        if (locked.critd_tbl(module_id) == crit_p) {
            T_E("module_id=%d name=%s about to create loop\n", module_id, name);
        }
        crit_p->nxt = locked.critd_tbl(module_id);
    }

    locked.critd_tbl(module_id, crit_p);

    // Initially, the critd is taken, so update the time for the first lock.
    // and pretend the user knows where this function is called from.
    crit_p->lock_tick_cnt            = vtss_current_time();
    crit_p->max_lock_tick_cnt        = 0;
    crit_p->total_lock_tick_cnt      = 0;
    crit_p->max_lock_thread_id       = vtss_thread_id_get();

    crit_p->init_done = 1;

    if (!create_locked) {
        critd_exit(crit_p, __FILE__, __LINE__, false);
    }
}

/******************************************************************************/
// critd_init()
/******************************************************************************/
void critd_init(critd_t *const crit_p, const char *const name,
                const vtss_module_id_t module_id,
                const critd_type_t type, const bool leaf)
{
    critd_init_(crit_p, name, module_id, type, leaf, false);
}

/******************************************************************************/
// critd_init_legacy()
/******************************************************************************/
void critd_init_legacy(critd_t *const crit_p, const char *const name,
                       const vtss_module_id_t module_id,
                       const critd_type_t type, const bool leaf)
{
    critd_init_(crit_p, name, module_id, type, leaf, true);
}

/******************************************************************************/
// critd_delete()
/******************************************************************************/
void critd_delete(critd_t *const crit_p)
{
    CritdTblLock locked;
    critd_t *i = nullptr, *prev = nullptr;

    // Find the entry located _before_ crit_p, as we need that to delete an
    // entry in a single linked list
    i = locked.critd_tbl(crit_p->module_id);
    while (i && i != crit_p) {
        prev = i;
        i = i->nxt;
    }

    if (!i) {
        T_E("Critd (%p, %s, %d) was not found in the monitor list!", crit_p, crit_p->name, crit_p->module_id);
        return;
    }

    if (!prev) {
        // No previous pointer means that the entry was found in front of the
        // list, this means that we need to update the head pointer.
        locked.critd_tbl(crit_p->module_id, crit_p->nxt);
    } else {
        prev->nxt = crit_p->nxt;
    }

    switch (crit_p->type) {
    case CRITD_TYPE_MUTEX:
    case CRITD_TYPE_MUTEX_RECURSIVE:
        vtss_mutex_destroy(&crit_p->m.mutex);
        break;

    case CRITD_TYPE_SEMAPHORE:
        vtss_sem_destroy(&crit_p->m.semaphore);
        break;

    default:
        VTSS_ASSERT(0);
    }

    vtss_flag_destroy(&crit_p->flag);
}

/******************************************************************************/
// critd_enter()
/******************************************************************************/
void critd_enter(critd_t *const crit_p, const char *const file, const int line, bool dry_run)
{
    int my_thread_id = vtss_thread_id_get();
    uint idx;

    if (!crit_p->init_done) {
        critd_trace(crit_p, file, line);
        VTSS_ASSERT(0);
    }

    // Assert that this thread has not already locked this critical region.
    // This check is only possible for mutexes. For semaphores, it's indeed
    // quite possible that the same thread attempts to take the semaphore twice
    // before the unlock (typically called from another thread) occurs. For
    // recursive mutexes, this must be allowed, that is the point in using a
    // recursive mutex.
    if (crit_p->type                   == CRITD_TYPE_MUTEX     &&
        crit_p->current_lock_thread_id != CRITD_THREAD_ID_NONE &&
        crit_p->current_lock_thread_id == my_thread_id) {
        critd_trace(crit_p, file, line);
        CRITD_ASSERT(0,  "Critical region already locked by this thread id!");
    }

    // Store information about lock attempt
    {
        CritdUpdateLock locked;
        ulong lock_cnt_new;
        lock_cnt_new = crit_p->lock_attempt_cnt;
        lock_cnt_new++;
        idx = (lock_cnt_new % CRITD_LOCK_ATTEMPT_SIZE);
        crit_p->lock_attempt_thread_id[idx] = my_thread_id;
        crit_p->lock_attempt_line[idx]      = line;
        crit_p->lock_attempt_file[idx]      = file;
        crit_p->lock_pending[idx]           = 1;
        crit_p->lock_attempt_cnt            = lock_cnt_new;
    }

    // It's safe to do the following without the critd mutex taken, because
    // leaf_crit_p is a per-thread entity, so nothing to protect.
    if (!leaf_detection_abuse_disabled && leaf_crit_p && leaf_crit_p != crit_p) {
        // Attempting to take lock of another mutex even though a leaf mutex
        // is currently taken.

        // We can only print one of these per session, because we have to
        // disable leaf-detection abuse, because otherwise, we cannot get to
        // print the trace error, get it saved to flash and make the LED blink,
        // because those functions also need mutexes.
        leaf_detection_abuse_disabled = true;
        T_E("A mutex (%s) is attempted taken while a leaf-mutex (%s) is held", crit_p->name, leaf_crit_p->name);
        critd_trace(crit_p,      file, line);
        critd_trace(leaf_crit_p, file, line);

        // Continue anyway, but now with leaf detection disabled.
    } else if (crit_p->leaf) {
        leaf_crit_p = crit_p;
    }

    // Here is the trick that causes the locking of all waiters until the very
    // first critd_exit() call has taken place. The flag is initially 0 and will
    // be set on the first call to critd_exit(), and remain set ever after,
    // causing this call to be fast.
    vtss_flag_wait(&crit_p->flag, 1, VTSS_FLAG_WAITMODE_OR);

    if (!dry_run) {
        switch (crit_p->type) {
        case CRITD_TYPE_MUTEX:
            vtss_mutex_lock(&crit_p->m.mutex);
            break;

        case CRITD_TYPE_SEMAPHORE:
            vtss_sem_wait(&crit_p->m.semaphore);
            break;

        case CRITD_TYPE_MUTEX_RECURSIVE:
            vtss_recursive_mutex_lock(&crit_p->m.rmutex);
            break;

        default:
            VTSS_ASSERT(0);
        }
    }

    if ((crit_p->type == CRITD_TYPE_MUTEX_RECURSIVE &&
         crit_p->m.rmutex.lock_cnt == 1) ||
        crit_p->type != CRITD_TYPE_MUTEX_RECURSIVE) {
        // Store information about lock
        crit_p->current_lock_thread_id = my_thread_id;
        crit_p->lock_thread_id         = my_thread_id;
        crit_p->lock_file              = file;
        crit_p->lock_line              = line;
        crit_p->lock_time              = time(NULL);
        crit_p->lock_tick_cnt          = vtss_current_time();
    }

    crit_p->lock_pending[idx] = 0;
    crit_p->lock_cnt++;
}

/******************************************************************************/
// critd_exit()
/******************************************************************************/
void critd_exit(critd_t *const crit_p, const char *const file, const int line, bool dry_run)
{
    int my_thread_id = vtss_thread_id_get();
    int recursive_mutex_lock_cnt_after = 0;

    if (vtss_flag_peek(&crit_p->flag) == 0) {
        // This is the very first call to critd_exit().
        // The call must open up for other threads waiting for the mutex.
        // This is accomplished by setting the event flag that others may
        // wait for in the critd_enter(). Once that is done, we simply return
        // because the mutex is not really taken, so we shouldn't unlock it.
        vtss_flag_setbits(&crit_p->flag, 1);

        // The very first time it's exited, we only update the time it took
        // and pretend that we know where it came from.
        crit_p->max_lock_file        = "critd_init";
        crit_p->max_lock_tick_cnt    = vtss_current_time() - crit_p->lock_tick_cnt;
        crit_p->total_lock_tick_cnt  = crit_p->max_lock_tick_cnt;

        // Since the mutex was already unlocked, nothing more to do here.
        return;
    }

    // Assert that critical region is currently locked
    if (!critd_is_locked(crit_p)) {
        critd_trace(crit_p, file, line);
        CRITD_ASSERT(0, "Unlock called, but critical region is not locked!");
    }

    // If it's a mutex or recursive mutex, the unlocking thread can never differ from the locking thread.
    // If it's a semaphore, this is indeed typically the case.
    if (crit_p->type == CRITD_TYPE_MUTEX ||
        crit_p->type == CRITD_TYPE_MUTEX_RECURSIVE) {
        // Assert that any current lock is this thread
        if (crit_p->lock_thread_id != CRITD_THREAD_ID_NONE &&
            crit_p->lock_thread_id != my_thread_id) {
            critd_trace(crit_p, file, line);
            CRITD_ASSERT(0, "Unlock called, but mutex is locked by different thread id!");
        }
    }

    if ((crit_p->type == CRITD_TYPE_MUTEX_RECURSIVE &&
         crit_p->m.rmutex.lock_cnt == 1) ||
        crit_p->type != CRITD_TYPE_MUTEX_RECURSIVE) {
        vtss_tick_count_t diff_ticks;

        // Clear current lock id
        crit_p->current_lock_thread_id = CRITD_THREAD_ID_NONE;

        // Store information about unlock
        crit_p->unlock_thread_id  = my_thread_id;
        crit_p->unlock_file       = file;
        crit_p->unlock_line       = line;

        diff_ticks = vtss_current_time() - crit_p->lock_tick_cnt;
        crit_p->total_lock_tick_cnt += diff_ticks;
        if (diff_ticks > crit_p->max_lock_tick_cnt) {
            crit_p->max_lock_tick_cnt  = diff_ticks;
            crit_p->max_lock_file      = crit_p->lock_file;
            crit_p->max_lock_line      = crit_p->lock_line;
            crit_p->max_lock_thread_id = crit_p->lock_thread_id;
        }
    }

    if (!dry_run) {
        switch (crit_p->type) {
        case CRITD_TYPE_MUTEX:
            vtss_mutex_unlock(&crit_p->m.mutex);
            break;

        case CRITD_TYPE_SEMAPHORE:
            vtss_sem_post(&crit_p->m.semaphore);
            break;

        case CRITD_TYPE_MUTEX_RECURSIVE:
            recursive_mutex_lock_cnt_after = vtss_recursive_mutex_unlock(&crit_p->m.rmutex);
            break;

        default:
            VTSS_ASSERT(0);
        }
    }

    if (!leaf_detection_abuse_disabled && crit_p->leaf) {
        if (!leaf_crit_p) {
            T_E("Internal error: Leaf mutex %s is getting released, but leaf_crit_p is NULL", crit_p->name);
        } else if (crit_p != leaf_crit_p) {
            T_E("Internal error: Leaf mutex (%s) held, but attempting to unlock another leaf mutex (%s)", leaf_crit_p->name, crit_p->name);
        } else if (crit_p->type == CRITD_TYPE_MUTEX_RECURSIVE && recursive_mutex_lock_cnt_after > 0) {
            // Do nothing, since we still have references to it.
        } else {
            leaf_crit_p = NULL;
        }
    }
}

/******************************************************************************/
// critd_assert_locked()
/******************************************************************************/
void critd_assert_locked(critd_t *const crit_p, const char *const file, const int line)
{
    if (critd_peek(crit_p) != 0) {
        critd_trace(crit_p, file, line);
        CRITD_ASSERT(0, "Critical region not locked!");
    }
}

/******************************************************************************/
// critd_dbg_list()
/******************************************************************************/
void critd_dbg_list(const critd_dbg_printf_t dbg_printf,
                    const vtss_module_id_t   module_id,
                    const BOOL               detailed,
                    const BOOL               header,
                    critd_t            *single_critd_p)
{
    CritdTblLock locked;
    critd_dbg_list_(locked, dbg_printf, module_id, detailed, header,
                    single_critd_p);
}

#ifdef VTSS_SW_OPTION_ICLI
/******************************************************************************/
// critd_dbg_list_icli()
/******************************************************************************/
void critd_dbg_list_icli(const u32              session_id,
                         const vtss_module_id_t module_id,
                         const BOOL             detailed,
                         const BOOL             header,
                         const critd_t          *single_critd_p)
{
    int                 i;
    const uint          MODULE_NAME_WID = 21;
    char                module_name_format[10];
    char                critd_name_format[10];
    vtss_module_id_t    mid_start = 0;
    vtss_module_id_t    mid_end = VTSS_MODULE_ID_NONE - 1;
    vtss_module_id_t    mid;
    int                 crit_cnt = 0;
    BOOL                first = header;

    /* Work-around for problem with printf("%*s", ...) */
    sprintf(module_name_format, "%%-%ds", MODULE_NAME_WID);
    sprintf(critd_name_format,  "%%-%ds", CRITD_NAME_LEN);

    if (module_id != VTSS_MODULE_ID_NONE) {
        mid_start = module_id;
        mid_end   = module_id;
    }

    for (mid = mid_start; mid <= mid_end; mid++) {
        const critd_t *critd_p;
        CritdTblLock locked;
        if (single_critd_p) {
            critd_p = single_critd_p;
        } else {
            critd_p = locked.critd_tbl(mid);
        }

        while (critd_p) {
            if (critd_p->cookie != CRITD_T_COOKIE) {
                T_E("Cookie=0x%08x, expected 0x%08x", critd_p->cookie, CRITD_T_COOKIE);
            }

            crit_cnt++;
            if (first) {
                if (!detailed) {
                    first = 0;
                }

                ICLI_PRINTF(module_name_format, "Module");
                ICLI_PRINTF(" ");
                ICLI_PRINTF(critd_name_format, "Critd Name");
                ICLI_PRINTF(" ");
                ICLI_PRINTF("T");
                ICLI_PRINTF(" ");
                ICLI_PRINTF("L");
                ICLI_PRINTF(" ");
                ICLI_PRINTF("State   "); // "Unlocked" or "Locked"
                ICLI_PRINTF(" ");
                ICLI_PRINTF("Lock Cnt  ");
                ICLI_PRINTF(" ");
                ICLI_PRINTF("LockTime ");
                ICLI_PRINTF("Latest Lock, Latest Unlock\n");
                for (i = 0; i < MODULE_NAME_WID; i++) {
                    ICLI_PRINTF("-");
                }

                ICLI_PRINTF(" ");
                for (i = 0; i < CRITD_NAME_LEN; i++) {
                    ICLI_PRINTF("-");
                }

                ICLI_PRINTF(" ");
                ICLI_PRINTF("- - -------- ---------- -------- ------------------------------\n");
            }

            ICLI_PRINTF(module_name_format, vtss_module_names[mid]);
            ICLI_PRINTF(" ");
            ICLI_PRINTF(critd_name_format, critd_p->name);
            ICLI_PRINTF(" ");
            ICLI_PRINTF("%c", critd_p->type == CRITD_TYPE_MUTEX ? 'M' : critd_p->type == CRITD_TYPE_MUTEX_RECURSIVE ? 'R' : 'S');
            ICLI_PRINTF(" ");
            ICLI_PRINTF("%c", critd_p->leaf ? 'Y' : 'N');
            ICLI_PRINTF(" ");
            if (critd_p->current_lock_thread_id == CRITD_THREAD_ID_NONE) {
                ICLI_PRINTF("Unlocked");
            } else {
                ICLI_PRINTF("Locked  ");
            }

            ICLI_PRINTF(" ");

            // Lock cnt
            ICLI_PRINTF("%10u ", critd_p->lock_cnt);

            {
                // Print lock time
                struct tm *timeinfo_p;
                struct tm timeinfo;
                time_t lock_time;

                lock_time = critd_p->lock_time;
#ifdef VTSS_SW_OPTION_SYSUTIL
                lock_time += (system_get_tz_off() * 60); /* Adjust for TZ minutes => seconds */
#endif
#ifdef VTSS_SW_OPTION_DAYLIGHT_SAVING
                lock_time += (time_dst_get_offset() * 60); /* Correct for DST */
#endif
                timeinfo_p = localtime_r(&lock_time, &timeinfo);

                ICLI_PRINTF("%02d:%02d:%02d ", timeinfo_p->tm_hour, timeinfo_p->tm_min, timeinfo_p->tm_sec);
            }
            {
                // Protect against concurrent update of lock information
                char file_lock[100] = "";
                char file_unlock[100] = "";
                strncpy(file_lock,   critd_p->lock_file   ? misc_filename(critd_p->lock_file)   : "",   98);
                strncpy(file_unlock, critd_p->unlock_file ? misc_filename(critd_p->unlock_file) : "", 98);
                file_lock[99]   = 0;
                file_unlock[99] = 0;

                ICLI_PRINTF("%lu/%s#%d, %lu/%s#%d",
                            critd_p->lock_thread_id,
                            file_lock,
                            critd_p->lock_line,
                            critd_p->unlock_thread_id,
                            file_unlock,
                            critd_p->unlock_line);
            }

            if (detailed) {
                int i;
                char file_lock[100] = "";
                int j = 0;
                ICLI_PRINTF(", lock_attempt_cnt=0x%lx", critd_p->lock_attempt_cnt);
                for (i = 0; i < CRITD_LOCK_ATTEMPT_SIZE; i++) {
                    if (j++ % 4 == 0) {
                        ICLI_PRINTF("\n  ");
                    }

                    strncpy(file_lock,   critd_p->lock_attempt_file[i] ? misc_filename(critd_p->lock_attempt_file[i])   : "",   98);
                    ICLI_PRINTF("[%x]=%lu/%s#%d%s ",
                                i,
                                critd_p->lock_attempt_thread_id[i],
                                file_lock,
                                critd_p->lock_attempt_line[i],
                                critd_p->lock_pending[i] ? "*" : " ");
                }
            }

            if (single_critd_p) {
                critd_p = NULL;
            } else {
                ICLI_PRINTF("\n");
                if (detailed) {
                    ICLI_PRINTF("\n");
                }

                critd_p = critd_p->nxt;
            }
        }
    }

    if (module_id != VTSS_MODULE_ID_NONE && crit_cnt == 0) {
    } else if (detailed && header) {
        ICLI_PRINTF("Note that the logging of lock attempts could be incorrect, since it is not protected by any critical region.\n");
        ICLI_PRINTF("However in most cases the information will likely be correct.\n");
        ICLI_PRINTF("\n");
        ICLI_PRINTF("The last logged lock attempt is stored in entry [lock_attempt_cnt %% %d].\n",
                    CRITD_LOCK_ATTEMPT_SIZE);
    }
}
#endif /* VTSS_SW_OPTION_ICLI */

#ifdef VTSS_SW_OPTION_ICLI
/******************************************************************************/
// critd_dbg_max_lock_icli()
/******************************************************************************/
void critd_dbg_max_lock_icli(const u32              session_id,
                             const vtss_module_id_t module_id,
                             const BOOL             header,
                             const BOOL             clear)
{
    int                 i;
    const uint          MODULE_NAME_WID = 21;
    char                module_name_format[10];
    char                critd_name_format[10];
    vtss_module_id_t    mid_start = 0;
    vtss_module_id_t    mid_end = VTSS_MODULE_ID_NONE - 1;
    vtss_module_id_t    mid;
    char                file_lock[100] = "";
    BOOL                first = header;

    /* Work-around for problem with printf("%*s", ...) */
    sprintf(module_name_format, "%%-%ds", MODULE_NAME_WID);
    sprintf(critd_name_format,  "%%-%ds", CRITD_NAME_LEN);

    if (module_id != VTSS_MODULE_ID_NONE) {
        mid_start = module_id;
        mid_end   = module_id;
    }

    for (mid = mid_start; mid <= mid_end; mid++) {
        CritdTblLock locked;
        critd_t *critd_p = locked.critd_tbl(mid);
        while (critd_p) {
            if (critd_p->cookie != CRITD_T_COOKIE) {
                T_E("Cookie=0x%08x, expected 0x%08x", critd_p->cookie, CRITD_T_COOKIE);
            }

            if (clear) {
                critd_p->max_lock_tick_cnt   = 0;
                critd_p->total_lock_tick_cnt = 0;
                critd_p->max_lock_file       = "";
                critd_p->max_lock_line       = 0;
                critd_p->max_lock_thread_id  = CRITD_THREAD_ID_NONE;
            } else {
                if (first) {
                    first = 0;
                    ICLI_PRINTF(module_name_format, "Module");
                    ICLI_PRINTF(" ");
                    ICLI_PRINTF(critd_name_format, "Critd Name");
                    ICLI_PRINTF(" T Conf MaxLock [s] Max Lock [ms] Tot Lock [ms] Lock Position\n");

                    for (i = 0; i < MODULE_NAME_WID; i++) {
                        ICLI_PRINTF("-");
                    }

                    ICLI_PRINTF(" ");

                    for (i = 0; i < CRITD_NAME_LEN; i++) {
                        ICLI_PRINTF("-");
                    }

                    ICLI_PRINTF(" ");
                    ICLI_PRINTF("- ---------------- ------------- ------------- ------------------------------\n");
                }

                ICLI_PRINTF(module_name_format, vtss_module_names[mid]);
                ICLI_PRINTF(" ");
                ICLI_PRINTF(critd_name_format, critd_p->name);
                ICLI_PRINTF(" %c", critd_p->type == CRITD_TYPE_MUTEX ? 'M' : critd_p->type == CRITD_TYPE_MUTEX_RECURSIVE ? 'R' : 'S');

                // Configured Max. lock time
                ICLI_PRINTF(" %16d", critd_p->max_lock_time);

                // Max. lock time
                ICLI_PRINTF(" " VPRI64Fu("13"), VTSS_OS_TICK2MSEC(critd_p->max_lock_tick_cnt));

                // Total lock time
                ICLI_PRINTF(" " VPRI64Fu("13") " ", VTSS_OS_TICK2MSEC(critd_p->total_lock_tick_cnt));

                // Lock file and line number
                // Protect against concurrent update of lock information
                strncpy(file_lock, critd_p->max_lock_file ? misc_filename(critd_p->max_lock_file) : "", 98);
                file_lock[99]   = 0;
                ICLI_PRINTF("%lu/%s#%d\n", critd_p->max_lock_thread_id, file_lock, critd_p->max_lock_line);
            }

            critd_p = critd_p->nxt;
        }
    }
}
#endif /* VTSS_SW_OPTION_ICLI */

/******************************************************************************/
// critd_module_init()
/******************************************************************************/
mesa_rc critd_module_init(vtss_init_data_t *data)
{
    mesa_rc rc = VTSS_RC_OK;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        // Create thread
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           critd_thread,
                           0,
                           "Critd",
                           nullptr,
                           0,
                           &critd_thread_handle,
                           &critd_thread_block);
        break;

    default:
        break;
    }

    return rc;
}

