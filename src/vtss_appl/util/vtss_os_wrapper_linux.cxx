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
#include "vtss_os_wrapper.h"
#include "thread"
#include "chrono.hxx"
#include <unistd.h>
#include "backtrace.hxx"
#include "critd_api.h"

// A little hack. This wrapper has no module ID, and - even worse - functions in
// this wrapper may be called and utilize trace prior to anything but the trace
// module itself has been trace-initialized, so we re-use that module's trace ID
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_TRACE
#include "vtss/basics/trace.hxx"
#include <zlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>    /* For SYS_xxx definitions */
#include <errno.h>
#include <vtss/basics/vector.hxx>
#include <vtss/basics/notifications/process-cmd.hxx>
#include <algorithm>
#include <termios.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <linux/sched.h>
#include <string>

#include <mbedtls/base64.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <mbedtls/entropy.h>
#include <mbedtls/aes.h>
#include <mbedtls/md.h>

#if defined(MSCC_BRSDK) // Internal CPU
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#define USE_MTD_FOR_CONFIG
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_SYSTEM

struct ThreadVecLock {
    ThreadVecLock(const char *file, unsigned int line)
    {
        if (!crit_.init_done) {
            // We do a lazy init, because we can't get our own mutex initialized
            // in any other way.
            critd_init(&crit_, "Thread Vector", VTSS_TRACE_MODULE_ID, CRITD_TYPE_MUTEX);
        }

        critd_enter(&crit_, file, line);
    }

    ~ThreadVecLock()
    {
        critd_exit(&crit_, __FILE__, __LINE__);
    }

private:
    static critd_t crit_;
};

critd_t ThreadVecLock::crit_;

#ifdef __cplusplus
extern "C" {
#endif

struct ThreadsInfo {
    ThreadsInfo(vtss_handle_t th, const char *n, int t, void *l)
        : thread_handle_(th), name_(n), tid_(t), any_location_on_thread_stack_(l) {}

    vtss_handle_t thread_handle() const
    {
        return thread_handle_;
    }

    const char *name() const
    {
        return name_;
    }

    int tid() const
    {
        return tid_;
    }

    void *any_location_on_thread_stack() const
    {
        return any_location_on_thread_stack_;
    }

private:
    vtss_handle_t thread_handle_;
    const char *name_;
    int tid_;
    void *any_location_on_thread_stack_;
};

/*---------------------------------------------------------------------------*/
// This contains a list of all threads created with vtss_thread_create().
// The purpose of it is to be able to 1) name a thread, 2) get
// the underlying Linux ID of the thread, and 3) the Posix thread ID.
static vtss::Vector<ThreadsInfo> thread_vec;

// This is a variable that is repeated per thread
static __thread int __vtss_thread_id;
int vtss_thread_id_get(void)
{
    if (__vtss_thread_id == 0) {
        __vtss_thread_id = syscall(SYS_gettid);
    }

    return __vtss_thread_id;
}

static void thread_vec_add(vtss_handle_t thread_handle, const char *name, int tid, volatile void *location_on_the_thread_stack)
{
    ThreadVecLock lock(__FILE__, __LINE__);

    auto itr = vtss::find_if(thread_vec.cbegin(), thread_vec.cend(), [&] (const ThreadsInfo & ti) {
        return ti.tid() > tid;
    });

    if (!thread_vec.emplace(itr, ThreadsInfo {thread_handle, name, tid, (void *)location_on_the_thread_stack})) {
        T_E("Unable to put thread '%s' in thread_vec", name);
    }
}

static void thread_vec_erase(vtss_handle_t thread_handle)
{
    ThreadVecLock lock(__FILE__, __LINE__);

    auto itr = vtss::find_if(thread_vec.cbegin(), thread_vec.cend(), [&] (const ThreadsInfo & ti) {
        return ti.thread_handle() == thread_handle;
    });

    if (itr != thread_vec.cend()) {
        T_D("Removing thread with ID = %d and name = %s from list of active threads", itr->tid(), itr->name());
        thread_vec.erase(itr);
    } else {
        T_W("No such thread found in thread_vec: 0x%lx", thread_handle);
    }
}

static int vtss_thread_vtss_prio_to_linux_prio(vtss_thread_prio_t tp)
{
    // On Linux, static thread priorities should be values between 1 and 99, but
    // only values between 1 and 32 are guaranteed to be supported on a given
    // platform, so we map our five required prios into these.
    // Higher value => higher priority.
    switch (tp) {
    case VTSS_THREAD_PRIO_BELOW_NORMAL_RT:
        return 4;

    case VTSS_THREAD_PRIO_DEFAULT_RT:
        return 5;

    case VTSS_THREAD_PRIO_ABOVE_NORMAL_RT:
        return 6;

    case VTSS_THREAD_PRIO_HIGH_RT:
        return 7;

    case VTSS_THREAD_PRIO_HIGHER_RT:
        return 8;

    case VTSS_THREAD_PRIO_HIGHEST_RT:
        return 9;

    default:
        T_E("Unable to convert %d to a pthread priority. Giving it low priority (3)", tp);
        return 3;
    }
}

static int vtss_thread_vtss_prio_to_linux_nice(vtss_thread_prio_t tp)
{
    // On Linux, niceness should be values in range -20 to 19, where higher number is higher
    // niceness (lower priority). Default is 0.
    switch (tp) {
    case VTSS_THREAD_PRIO_BELOW_NORMAL:
        return 5;

    case VTSS_THREAD_PRIO_DEFAULT:
        return 0;

    case VTSS_THREAD_PRIO_ABOVE_NORMAL:
        return -5;

    case VTSS_THREAD_PRIO_HIGH:
        return -10;

    case VTSS_THREAD_PRIO_HIGHER:
        return -15;

    case VTSS_THREAD_PRIO_HIGHEST:
        return -20;

    default:
        T_E("Unable to convert %d to a pthread priority. Giving it low niceness (1)", tp);
        return 1;
    }
}


static vtss_thread_prio_t vtss_thread_linux_rt_prio_to_vtss_prio(int linux_prio)
{
    switch (linux_prio) {
    case 4:
        return VTSS_THREAD_PRIO_BELOW_NORMAL_RT;

    case 5:
        return VTSS_THREAD_PRIO_DEFAULT_RT;

    case 6:
        return VTSS_THREAD_PRIO_ABOVE_NORMAL_RT;

    case 7:
        return VTSS_THREAD_PRIO_HIGH_RT;

    case 8:
        return VTSS_THREAD_PRIO_HIGHER_RT;

    case 9:
        return VTSS_THREAD_PRIO_HIGHEST_RT;

    default:
        return VTSS_THREAD_PRIO_NA;
    }
}

static vtss_thread_prio_t vtss_thread_linux_nice_to_vtss_prio(int linux_nice)
{
    switch (linux_nice) {
    case 5:
        return VTSS_THREAD_PRIO_BELOW_NORMAL;

    case 0:
        return VTSS_THREAD_PRIO_DEFAULT;

    case -5:
        return VTSS_THREAD_PRIO_ABOVE_NORMAL;

    case -10:
        return VTSS_THREAD_PRIO_HIGH;

    case -15:
        return VTSS_THREAD_PRIO_HIGHER;

    case -20:
        return VTSS_THREAD_PRIO_HIGHEST;

    default:
        return VTSS_THREAD_PRIO_NA;
    }
}

static vtss_thread_prio_t vtss_thread_linux_prio_to_vtss_prio(BOOL is_rt, int prio_nice)
{
    return is_rt ? vtss_thread_linux_rt_prio_to_vtss_prio(prio_nice) : vtss_thread_linux_nice_to_vtss_prio(prio_nice);
}

typedef struct {
    vtss_thread_entry_f *user_thread_func;
    vtss_addrword_t      user_thread_func_arg;
    const char           *thread_name;
    vtss_thread_prio_t   priority;
} thread_create_data_t;

static void *thread_create_wrapper_func(vtss_addrword_t data)
{
    vtss_handle_t        thread_handle = vtss_thread_self();
    thread_create_data_t *user_data    = (thread_create_data_t *)data;
    volatile int         tid           = syscall(SYS_gettid); // gettid() is not available in glibc.

    // In order for the application to iterate over all threads and
    // give the thread a name, we add it to our own map of threads.
    // Also, we use Linux' thread ID (tid) as a thread ID.
    // For debugging, we also give it a pointer to any location on the stack
    // ('tid' is declared volatile to enforce the compiler to allocate it on the stack).
    T_I("Thread creation wrapper invoked for %s. Adding TID = %d to thread vector", user_data->thread_name, tid);
    thread_vec_add(thread_handle, user_data->thread_name, tid, &tid);

    // Change our own priority. The thread creator couldn't set it,
    // because we're created with inherited scheduler.
    T_I("Setting priority of %s to %d", user_data->thread_name, user_data->priority);
    vtss_thread_prio_set(thread_handle, user_data->priority);

    // And call the user's entry function with the user's entry data.
    user_data->user_thread_func(user_data->user_thread_func_arg);

    // Now that we are exiting, remove us from the thread_map.
    T_I("User-thread function (%s) is returning. Deleting ourselves from thread map", user_data->thread_name);
    thread_vec_erase(thread_handle);

    // user_data was allocated by vtss_thread_create(). No longer needed.
    VTSS_FREE(user_data);

    return nullptr;
}

// "Local" Trace Error
// When trace module hasn't been initialized T_E() doesn't output anything. Therefore
// we use the LT_E() function to use both trace module and a plain old printf.
#define LT_E(_fmt_, ...)              \
    do {                              \
        T_E(_fmt_, ##__VA_ARGS__);    \
        printf("Error: ");            \
        printf(_fmt_, ##__VA_ARGS__); \
        printf("\n");                 \
    } while (0)

/**
 * vtss_thread_prio_is_rt()
 * Returns TRUE if #prio is one of the real-time priorities,
 * FALSE otherwise.
 */
static BOOL vtss_thread_prio_is_rt(vtss_thread_prio_t prio)
{
    switch (prio) {
    case VTSS_THREAD_PRIO_BELOW_NORMAL_RT:
    case VTSS_THREAD_PRIO_DEFAULT_RT:
    case VTSS_THREAD_PRIO_ABOVE_NORMAL_RT:
    case VTSS_THREAD_PRIO_HIGH_RT:
    case VTSS_THREAD_PRIO_HIGHER_RT:
    case VTSS_THREAD_PRIO_HIGHEST_RT:
        return TRUE;

    default:
        break;
    }

    return FALSE;
}

/**
 * vtss_thread_prio_is_not_rt()
 * Returns TRUE if #prio is NOT one of the real-time priorities,
 * FALSE otherwise.
 */
static BOOL vtss_thread_prio_is_not_rt(vtss_thread_prio_t prio)
{
    switch (prio) {
    case VTSS_THREAD_PRIO_BELOW_NORMAL:
    case VTSS_THREAD_PRIO_DEFAULT:
    case VTSS_THREAD_PRIO_ABOVE_NORMAL:
    case VTSS_THREAD_PRIO_HIGH:
    case VTSS_THREAD_PRIO_HIGHER:
    case VTSS_THREAD_PRIO_HIGHEST:
        return TRUE;

    default:
        break;
    }

    return FALSE;
}

static int vtss_thread_id_from_handle(vtss_handle_t handle)
{
    ThreadVecLock lock(__FILE__, __LINE__);

    auto itr = vtss::find_if(thread_vec.cbegin(), thread_vec.cend(), [&] (const ThreadsInfo & ti) {
        return ti.thread_handle() == handle;
    });

    if (itr != thread_vec.cend()) {
        return itr->tid();
    }

    T_W("Thread handle not found: %lu", handle);

    return 0;
}

void vtss_thread_prio_set(vtss_handle_t thread_handle, vtss_thread_prio_t prio)
{
    int                res, policy;
    struct sched_param sched_param;
    BOOL               is_rt = vtss_thread_prio_is_rt(prio);
    BOOL               is_not_rt = vtss_thread_prio_is_not_rt(prio);

    if (!is_rt && !is_not_rt) {
        LT_E("Invalid priority %d", prio);
        return;
    }

    if (is_rt && is_not_rt) {
        LT_E("Internal error %d", prio);
        return;
    }

    if (is_rt) {
        // For real-time threads, we can selected between SCHED_FIFO and SCHED_RR
        // scheduling policy.
        // Real-time threads take precedence over threads created with
        // SCHED_OTHER/SCHED_BATCH/SCHED_IDLE.
        // SCHED_FIFO is a simple first-in-first-out scheduling without
        // time-slicing. A thread may occupy the CPU for as long as it likes.
        // SCHED_RR time slices, which is why we use that one. The time slice is 25 ms
        // (can be seen in /proc/sys/kernel/sched_rr_timeslice_ms).
        policy = SCHED_RR;

        // NOTICE: Setting the priority to a non-zero value (as we do in this application),
        // requires the application to run as root or to be granted the CAP_SYS_NICE property.
        sched_param.sched_priority = vtss_thread_vtss_prio_to_linux_prio(prio);
    } else {
        // For non-real time threads, we select SCHED_NORMAL and use "niceness"
        // to control "priority" (actually it's the time-slice length and not
        // the priority we control).
        policy = SCHED_NORMAL;
        sched_param.sched_priority = 0; // Must be 0 for non-RT threads.
    }

    if ((res = pthread_setschedparam(thread_handle, policy, &sched_param)) != 0) {
        LT_E("pthread_setschedparam(): %s", strerror(res));
        // Continue anyway
    }

    if (!is_rt) {
        if (setpriority(PRIO_PROCESS, vtss_thread_id_from_handle(thread_handle), vtss_thread_vtss_prio_to_linux_nice(prio)) < 0) {
            LT_E("setpriority(): %s", strerror(errno));
        }
    }
}

void vtss_thread_create(vtss_thread_prio_t  priority,       // Thread priority (VTSS_THREAD_PRIO_xxx; e.g. VTSS_THREAD_PRIO_DEFAULT)
                        vtss_thread_entry_f *entry,         // Entry point function
                        vtss_addrword_t     entry_data,     // Entry data
                        const char          *name,          // Thread name
                        void                *stack_base,    // Stack base
                        u32                 stack_size,     // Stack size
                        vtss_handle_t       *thread_handle, // Returned thread handle
                        vtss_thread_t       *thread)        // Put thread here (not used)
{
    static bool          thread_create_initialized;
    pthread_attr_t       attr;
    int                  res;
    thread_create_data_t *user_data;

    // Make sure we can call pthread_attr_destroy() on the attributes
    // when we exit, by initalizing them first.
    if ((res = pthread_attr_init(&attr)) != 0) {
        LT_E("pthread_attr_init(): %s", strerror(res));
        return;
    }

    vtss_global_lock(__FILE__, __LINE__);
    if (!thread_create_initialized) {
        volatile int  tid, pid;
        vtss_handle_t main_thread_handle;

        // It's the very first time this function is
        // invoked. We need to do a few things.

        tid = vtss_thread_id_get();
        pid = getpid();

        // If the Thread ID is equal to the Process ID, this indeed is the
        // main thread that's invoking us. Otherwise, someone else has
        // create a thread without using this thread creation wrapper, and
        // that thread is invoking us.
        if (tid != pid) {
            LT_E("The first thread invoking us is not the main thread (expected TID = %d), but thread with ID = %d", pid, tid);
            vtss_global_unlock(__FILE__, __LINE__);
            goto do_exit;
        }

        main_thread_handle = vtss_thread_self();

        // Add the main thread to our internal list of threads
        thread_vec_add(main_thread_handle, "Main", tid, &tid);

        // Set the scheduling policy and priority of the main thread.
        vtss_thread_prio_set(main_thread_handle, VTSS_THREAD_PRIO_DEFAULT);

        thread_create_initialized = true;
    }

    vtss_global_unlock(__FILE__, __LINE__);

    // Now it's time to create the new thread.

    // Argument checks
    if (name == nullptr) {
        LT_E("No thread name supplied");
        goto do_exit;
    }

    if (entry == nullptr) {
        LT_E("No entry function supplied for thread named %s", name);
        goto do_exit;
    }

    if (thread_handle == nullptr) {
        LT_E("#thread_handle is NULL for thread named %s", name);
        goto do_exit;
    }

    *thread_handle = 0;

#if 0
    // If this code is included, we'll use each thread's own pre-allocated stack, which prevents
    // overcommitting. If it's not included, we'll let pthread determine the max thread size.
    // The latter could be useful for debugging in case of a stack overflow. Use 'debug pagemap' and
    // search for [stack:<thread_id>] to figure out how much stack is actually used.
    if (stack_size && stack_base) {
        if ((res = pthread_attr_setstack(&attr, stack_base, MAX(PTHREAD_STACK_MIN, stack_size))) != 0) {
            LT_E("pthread_attr_setstack(): %s", strerror(res));
            goto do_exit;
        }
    } else {
        LT_E("Thread %s hasn't set a stack size. Using Linux default", name);
    }
#endif

    // Create the thread detached so that the thread creating it won't have
    // to wait for the new thread to terminate.
    if ((res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0) {
        LT_E("pthread_attr_setdetachstate(): %s", strerror(res));
        goto do_exit;
    }

    // Start by inheriting the invoking thread's scheduler properties. Inside
    // the thread creation wrapper, the real scheduling policy and priority is set.
    if ((res = pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED)) != 0) {
        LT_E("pthread_attr_setinheritsched(): %s", strerror(res));
        goto do_exit;
    }

    // Wrap the user's thread entry point into our own thread so that we
    // can obtain and save the thread's ID in our internal thread list.
    if ((VTSS_MALLOC_CAST(user_data, sizeof(thread_create_data_t))) == NULL) {
        LT_E("Unable to allocate " VPRIz " bytes for user data", sizeof(thread_create_data_t));
        goto do_exit;
    }

    user_data->user_thread_func     = entry;
    user_data->user_thread_func_arg = entry_data;
    user_data->thread_name          = name;
    user_data->priority             = priority; // The thread sets its own priority. We can't do that up front, because we might need the thread's ID.

    T_I("Invoking pthread_create() for %s (priority = %d)", name, user_data->priority);

    // Time to create the thread.
    if ((res = pthread_create(thread_handle, &attr, thread_create_wrapper_func, (vtss_addrword_t)user_data)) != 0) {
        LT_E("pthread_create(): %s", strerror(res));
        goto do_exit;
    }

    T_D("Created thread with name = %s", name);

do_exit:
    // Free the attributes prior to returning (upon call to pthread_create(), the
    // attributes were copied into the new thread, so it's safe to destroy these ones).
    (void)pthread_attr_destroy(&attr);
}

// It may be necessary to arrange for the victim to run for it to disappear
// Returns false if not deleted
vtss_bool_t vtss_thread_delete(vtss_handle_t thread_handle)
{
    int res;
    bool result = true;

    T_I("Force-delete of a thread");

    // Before cancelling the thread, remove it from the thread map to avoid
    // race conditions.
    thread_vec_erase(thread_handle);

    // The thread must be cooperative for this to work.
    if ((res = pthread_cancel(thread_handle)) != 0) {
        T_E("pthread_cancel(): %s", strerror(res));
        result = false;
    }

    return result;
}

void vtss_thread_yield(void)
{
    // Relinquish the CPU
    sched_yield();
}

vtss_handle_t vtss_thread_self(void)
{
    return pthread_self();
}

// Thread priority manipulation
static void thread_os_prio_get(vtss_handle_t thread_handle, BOOL *is_rt, int *prio_nice)
{
    int                policy, res;
    struct sched_param sched_param;

    *is_rt     = 0;
    *prio_nice = 0;

    if ((res = pthread_getschedparam(thread_handle, &policy, &sched_param)) != 0) {
        T_E("pthread_getschedparam(): %s", strerror(res));
        return;
    }

    *is_rt = policy == SCHED_RR || policy == SCHED_FIFO;

    if (*is_rt) {
        *prio_nice = sched_param.sched_priority;
    } else {
        int tid = vtss_thread_id_from_handle(thread_handle);

        errno = 0;
        *prio_nice = getpriority(PRIO_PROCESS, tid);

        if (errno) {
            T_E("getpriority(%d): %s", tid, strerror(errno));
        }
    }
}

static bool thread_stack_attr_get(vtss_handle_t thread_handle, void **stack_base, size_t *stack_size)
{
    pthread_attr_t attr;
    int            res;
    bool           result = true;

    if ((res = pthread_getattr_np(thread_handle, &attr)) != 0) {
        T_E("pthread_getattr_np(): %s", strerror(res));
        return false;
    }

    if ((res = pthread_attr_getstack(&attr, stack_base, stack_size)) != 0) {
        T_E("pthread_attr_getstack(): %s", strerror(res));
        result = false;
    }

    if ((res = pthread_attr_destroy(&attr)) != 0) {
        T_E("pthread_attr_destroy(). %s", strerror(res));
    }

    return result;
}

vtss_bool_t vtss_thread_info_get(vtss_handle_t thread_handle, vtss_thread_info_t *info)
{
    if (info == nullptr) {
        T_E("#info is NULL");
        return false;
    }

    memset(info, 0, sizeof(*info));

    {
        ThreadVecLock lock(__FILE__, __LINE__);
        auto itr = vtss::find_if(thread_vec.cbegin(), thread_vec.cend(), [&] (const ThreadsInfo & ti) {
            return ti.thread_handle() == thread_handle;
        });

        if (itr == thread_vec.end()) {
            return false;
        }

        info->handle = itr->thread_handle();
        info->tid    = itr->tid();
        info->name   = itr->name();
    }

    thread_os_prio_get(thread_handle, &info->rt, &info->os_prio);
    info->prio = vtss_thread_linux_prio_to_vtss_prio(info->rt, info->os_prio);
    return thread_stack_attr_get(thread_handle, &info->stack_base, &info->stack_size);
}

vtss_handle_t vtss_thread_get_next(vtss_handle_t thread_handle)
{
    ThreadVecLock lock(__FILE__, __LINE__);

    if (thread_handle == 0) {
        // Starting all over

        if (thread_vec.empty()) {
            return 0;
        }

        return thread_vec.cbegin()->thread_handle();
    }

    auto itr = vtss::find_if(thread_vec.cbegin(), thread_vec.cend(), [&] (const ThreadsInfo & ti) {
        return ti.thread_handle() == thread_handle;
    });

    if (itr == thread_vec.cend()) {
        return 0;
    }

    if (++itr == thread_vec.cend()) {
        return 0;
    }

    return itr->thread_handle();
}

vtss_handle_t vtss_thread_handle_from_id(int id)
{
    ThreadVecLock lock(__FILE__, __LINE__);

    auto itr = vtss::find_if(thread_vec.cbegin(), thread_vec.cend(), [&] (const ThreadsInfo & ti) {
        return ti.tid() == id;
    });

    if (itr != thread_vec.cend()) {
        return itr->thread_handle();
    }

    T_W("Thread ID not found: %d", id);

    return 0;
}

int vtss_thread_id_from_thread_stack_range_get(void *lo, void *hi)
{
    ThreadVecLock lock(__FILE__, __LINE__);

    auto itr = vtss::find_if(thread_vec.cbegin(), thread_vec.cend(), [&] (const ThreadsInfo & ti) {
        void *l = ti.any_location_on_thread_stack();
        return l >= lo && l < hi;
    });

    if (itr != thread_vec.cend()) {
        return itr->tid();
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/* thread speicific data                                                     */

vtss_thread_key_t vtss_thread_new_data_index(void)
{
    vtss_thread_key_t key;
    VTSS_ASSERT(pthread_key_create(&key, NULL) == 0);
    return key;
}

vtss_addrword_t vtss_thread_get_data(vtss_thread_key_t index)
{
    void    *data;

    if ((data = pthread_getspecific(index)) == NULL) {
        T_E("Failed to get thread specific data");
    }

    return (vtss_addrword_t)data;
}

void vtss_thread_set_data(vtss_thread_key_t index, vtss_addrword_t data)
{
    if (pthread_setspecific(index, (void *)data) != 0) {
        T_E("Failed to set thread specific data");
    }
}

/*---------------------------------------------------------------------------*/
/* Clocks and Alarms                                                         */

// Returns value of real time clock's counter.
vtss_tick_count_t vtss_current_time(void)
{
    return VTSS_OS_MSEC2TICK(vtss::uptime_milliseconds());
}

/*---------------------------------------------------------------------------*/
/* Mutex                                                                     */

static void mutex_init(vtss_mutex_t *mutex, bool recursive)
{
    pthread_mutexattr_t attr;
    int                 res;

    memset(mutex, 0, sizeof(*mutex));

    if ((res = pthread_mutexattr_init(&attr)) != 0) {
        T_E("pthread_mutexattr_init(): %s", strerror(res));
        return;
    }

    // When a higher-priority thread is waiting for a mutex owned
    // by a lower-priority thread, boost the lower-priority thread
    // to the priority of the higher-priority thread.
    if ((res = pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT)) != 0) {
        T_E("pthread_mutexattr_setprotocol(): %s", strerror(res));
        goto do_exit;
    }

    // Recursive?
    if (recursive) {
        if ((res = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE)) != 0) {
            T_E("pthread_mutexattr_settype(): %s", strerror(res));
            goto do_exit;
        }
    }

    if ((res = pthread_mutex_init(mutex, &attr)) != 0) {
        T_E("pthread_mutex_init(): %s", strerror(res));
        goto do_exit;
    }

do_exit:
    // Destroy the attributes again (they are copied in the call to pthread_mutex_init())
    if ((res = pthread_mutexattr_destroy(&attr)) != 0) {
        T_E("pthread_mutexattr_destroy(): %s", strerror(res));
    }
}

void vtss_mutex_init(vtss_mutex_t *mutex)
{
    mutex_init(mutex, false);
}

void vtss_mutex_destroy(vtss_mutex_t *mutex)
{
    pthread_mutex_destroy(mutex);
    memset(mutex, 0, sizeof(*mutex));
}

vtss_bool_t vtss_mutex_lock(vtss_mutex_t *mutex)
{
    return pthread_mutex_lock(mutex) == 0;
}

vtss_bool_t vtss_mutex_trylock(vtss_mutex_t *mutex)
{
    return pthread_mutex_trylock(mutex) == 0;
}

void vtss_mutex_unlock(vtss_mutex_t *mutex)
{
    pthread_mutex_unlock(mutex);
}

/*---------------------------------------------------------------------------*/
/* Recursive Mutex                                                           */
void vtss_recursive_mutex_init(vtss_recursive_mutex_t *mutex)
{
    // Initialize a pthread_mutex to be recursive
    memset(mutex, 0x0, sizeof(vtss_recursive_mutex_t));

    mutex_init(&mutex->mutex_, true);
}

void vtss_recursive_mutex_destroy(vtss_recursive_mutex_t *mutex)
{
    pthread_mutex_destroy(&(mutex->mutex_));
}

vtss_bool_t vtss_recursive_mutex_lock(vtss_recursive_mutex_t *mutex)
{
    // The mutex is initialized as a recursive mutex meaning that we are free to
    // call pthread_mutex_lock multiple times.
    if (pthread_mutex_lock(&(mutex->mutex_)) == 0) {
        mutex->lock_cnt++;
        return true;
    }

    return false;
}

vtss_bool_t vtss_recursive_mutex_trylock(vtss_recursive_mutex_t *mutex)
{
    // Greatly simplified because the underlaying mutex is recursive
    if (pthread_mutex_trylock(&(mutex->mutex_)) == 0) {
        mutex->lock_cnt++;
        return true;
    }

    return false;
}

int vtss_recursive_mutex_unlock(vtss_recursive_mutex_t *mutex)
{
    // The mutex is initialized as a recusrive mutex meaning that we are free to
    // call pthread_mutex_unlock multiple times.
    int lock_cnt_after;

    if (mutex->lock_cnt <= 0) {
        T_E("Unexpected lock_cnt:%d", mutex->lock_cnt);
        return 0;
    }

    mutex->lock_cnt--;
    lock_cnt_after = mutex->lock_cnt;

    int res = pthread_mutex_unlock(&(mutex->mutex_));
    VTSS_ASSERT(res == 0);

    return lock_cnt_after;
}

/*---------------------------------------------------------------------------*/
/* Condition Variables                                                       */

void vtss_cond_init(vtss_cond_t *cond, vtss_mutex_t *mutex)
{
    pthread_condattr_t attr;
    int                res;

    if (mutex == NULL || cond == NULL) {
        T_E("Invalid args");
        return;
    }

    if ((res = pthread_condattr_init(&attr)) != 0) {
        T_E("pthread_condattr_init(): %s", strerror(res));
        return;
    }

    if ((res = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC)) != 0) {
        T_E("pthread_condattr_setclock(): %s", strerror(res));
        goto do_exit;
    }

    memset(cond, 0, sizeof(*cond));
    cond->mutex = mutex;

    if ((res = pthread_cond_init(&cond->cond, &attr)) != 0) {
        T_E("pthread_cond_init(): %s", strerror(res));
        goto do_exit;
    }

do_exit:
    if ((res = pthread_condattr_destroy(&attr)) != 0) {
        T_E("pthread_condattr_destroy(): %s", strerror(res));
    }
}

void vtss_cond_destroy(vtss_cond_t *cond)
{
    pthread_cond_destroy(&cond->cond);
    memset(cond, 0, sizeof(*cond));
}

vtss_bool_t vtss_cond_wait(vtss_cond_t *cond)
{
    int res;

    if ((res = pthread_cond_wait(&cond->cond, cond->mutex)) != 0) {
        T_E("pthread_cond_wait(): %s", strerror(res));
    }

    return true;
}

void vtss_cond_signal(vtss_cond_t *cond)
{
    pthread_cond_signal(&cond->cond);
}

void vtss_cond_broadcast(vtss_cond_t *cond)
{
    pthread_cond_broadcast(&cond->cond);
}

vtss_bool_t vtss_cond_timed_wait(vtss_cond_t *cond, vtss_tick_count_t abstime)
{
    struct timespec ts;
    u64             abstime_ms;
    int             res;

    if (abstime <= vtss_current_time()) {
        // Timed out
        return false;
    }

    abstime_ms = VTSS_OS_TICK2MSEC(abstime);

    ts.tv_sec  = abstime_ms / 1000;
    ts.tv_nsec = 1000 * 1000 * (abstime_ms % 1000);

    T_N("vtss_cond_timed_wait() for " VPRI64u "ms", abstime_ms);

    res = pthread_cond_timedwait(&cond->cond, cond->mutex, &ts);

    if (res == ETIMEDOUT) {
        return false;
    }

    if (res != 0) {
        T_E("pthread_cond_timedwait(): %s", strerror(res));
        return false;
    }

    return true;
}

/*---------------------------------------------------------------------------*/
/* Flags                                                                     */
void vtss_flag_init(vtss_flag_t *flag)
{
    vtss_mutex_init(&flag->mutex);
    vtss_cond_init(&flag->cond, &flag->mutex);
    flag->flags = 0;
}

void vtss_flag_destroy(vtss_flag_t *flag)
{
    vtss_cond_destroy(&flag->cond);
    vtss_mutex_destroy(&flag->mutex);
}

// Bit-wise OR in the bits in value; awaken any waiting tasks whose
// condition is now satisfied.
void vtss_flag_setbits(vtss_flag_t *flag, vtss_flag_value_t value)
{
    vtss_mutex_lock(&flag->mutex);

    if (value) {
        flag->flags |= value;
        vtss_cond_broadcast(&flag->cond);
    }

    vtss_mutex_unlock(&flag->mutex);
}

// Bit-wise AND with the the bits in value; this clears the bits which
// are not set in value. No waiting task can be awoken.
void vtss_flag_maskbits(vtss_flag_t *flag, vtss_flag_value_t value)
{
    vtss_mutex_lock(&flag->mutex);
    flag->flags &= value;
    vtss_mutex_unlock(&flag->mutex);
}

static bool flag_wait_done(vtss_flag_value_t flags, vtss_flag_value_t pattern, vtss_flag_mode_t mode)
{
    if (mode == VTSS_FLAG_WAITMODE_OR || mode == VTSS_FLAG_WAITMODE_OR_CLR) {
        return (flags & pattern) != 0;
    }

    // AND - all must be set
    return (flags & pattern) == pattern;
}

// Wait for the flag value to match the pattern, according to the mode.
// If mode includes CLR, set the flag value to zero when
// our pattern is matched.  The return value is that which matched
// the request, or zero for an error/timeout return.
// #pattern must not itself be zero.
vtss_flag_value_t vtss_flag_wait(vtss_flag_t *flag, vtss_flag_value_t pattern, vtss_flag_mode_t mode)
{
    vtss_flag_value_t result;
    vtss_mutex_lock(&flag->mutex);

    while (!flag_wait_done(flag->flags, pattern, mode)) {
        (void)vtss_cond_wait(&flag->cond);
    }

    result = flag->flags;

    if (mode == VTSS_FLAG_WAITMODE_AND_CLR || mode == VTSS_FLAG_WAITMODE_OR_CLR) {
        flag->flags = 0;
    }

    vtss_mutex_unlock(&flag->mutex);

    return result;
}

vtss_flag_value_t vtss_flag_timed_wait(vtss_flag_t       *flag,
                                       vtss_flag_value_t pattern,
                                       vtss_flag_mode_t  mode,
                                       vtss_tick_count_t abstime)
{
    vtss_flag_value_t result;
    bool              timed_out = false;

    vtss_mutex_lock(&flag->mutex);

    while (!flag_wait_done(flag->flags, pattern, mode)) {
        if (!vtss_cond_timed_wait(&flag->cond, abstime)) {
            // Timed out
            timed_out = true;
            break;
        }
    }

    if (timed_out) {
        result = 0;
    } else {
        result = flag->flags;

        if (mode == VTSS_FLAG_WAITMODE_AND_CLR || mode == VTSS_FLAG_WAITMODE_OR_CLR) {
            flag->flags = 0;
        }
    }

    vtss_mutex_unlock(&flag->mutex);

    return result;
}

vtss_flag_value_t vtss_flag_timed_waitfor(vtss_flag_t       *flag,
                                          vtss_flag_value_t pattern,
                                          vtss_flag_mode_t  mode,
                                          vtss_tick_count_t reltime)
{
    return vtss_flag_timed_wait(flag, pattern, mode, vtss_current_time() + reltime);
}

vtss_flag_value_t vtss_flag_poll(vtss_flag_t       *flag,
                                 vtss_flag_value_t pattern,
                                 vtss_flag_mode_t  mode)
{
    vtss_flag_value_t result;
    vtss_mutex_lock(&flag->mutex);

    if (flag_wait_done(flag->flags, pattern, mode)) {

        result = flag->flags;

        if (mode == VTSS_FLAG_WAITMODE_AND_CLR || mode == VTSS_FLAG_WAITMODE_OR_CLR) {
            flag->flags = 0;
        }
    } else {
        result = 0;
    }

    vtss_mutex_unlock(&flag->mutex);

    return result;
}

vtss_flag_value_t vtss_flag_peek(vtss_flag_t *flag)
{
    vtss_flag_value_t result;

    vtss_mutex_lock(&flag->mutex);
    result = flag->flags;
    vtss_mutex_unlock(&flag->mutex);

    return result;
}

/*---------------------------------------------------------------------------*/
/* Semaphores                                                                */
void vtss_sem_init(vtss_sem_t *sem, u32 val)
{
    memset(sem, 0, sizeof(*sem));
    vtss_mutex_init(&sem->mutex);
    vtss_cond_init(&sem->cond, &sem->mutex);
    sem->count = val;
}

void vtss_sem_destroy(vtss_sem_t *sem)
{
    vtss_cond_destroy(&sem->cond);
    vtss_mutex_destroy(&sem->mutex);
}

void vtss_sem_wait(vtss_sem_t *sem)
{
    vtss_mutex_lock(&sem->mutex);

    while (sem->count == 0) {
        (void)vtss_cond_wait(&sem->cond);
    }

    sem->count--;
    vtss_mutex_unlock(&sem->mutex);
}

vtss_bool_t vtss_sem_timed_wait(vtss_sem_t *sem, vtss_tick_count_t abstime)
{
    vtss_bool_t timed_out = false;

    vtss_mutex_lock(&sem->mutex);

    while (sem->count == 0) {
        if (!vtss_cond_timed_wait(&sem->cond, abstime)) {
            // Timed out
            timed_out = true;
            break;
        }
    }

    if (!timed_out) {
        sem->count--;
    }

    vtss_mutex_unlock(&sem->mutex);

    return !timed_out;
}

vtss_bool_t vtss_sem_trywait(vtss_sem_t *sem)
{
    return vtss_sem_timed_wait(sem, 0);
}

void vtss_sem_post(vtss_sem_t *sem, u32 increment_by)
{
    vtss_mutex_lock(&sem->mutex);
    sem->count += increment_by;
    vtss_cond_signal(&sem->cond);
    vtss_mutex_unlock(&sem->mutex);
}

u32 vtss_sem_peek(vtss_sem_t *sem)
{
    u32 result;

    vtss_mutex_lock(&sem->mutex);
    result = sem->count;
    vtss_mutex_unlock(&sem->mutex);

    return result;
}

u64 hal_time_get(void)
{
    return vtss::uptime_microseconds();
}

#ifdef __cplusplus
}
#endif

// Flash operations

#include "flash_mgmt_api.h"

typedef struct {
    const char             *name;
    const char             *fn;
    size_t                 len;
    const struct flash_ops *fop;
    vtss_flashaddr_t       data;
} flash_section_t;

struct flash_ops {
    BOOL (*init)   (flash_section_t *item, flash_mgmt_section_info_t *info);
    int  (*read)   (flash_section_t *item, vtss_flashaddr_t flash_addr, void *ram_addr, size_t len);
    int  (*program)(flash_section_t *item, vtss_flashaddr_t flash_addr, const void *ram_addr, size_t len);
    int  (*erase)  (flash_section_t *item, vtss_flashaddr_t flash_addr, size_t len);
};

// Emulated flash ---------------------
static BOOL file_flash_init(flash_section_t *item, flash_mgmt_section_info_t *info)
{
    BOOL ret = FALSE;
    int  fd;

    T_I("Init(enter, %s)",  item->name);

    if (item->data) {
        // Already initialized
        return TRUE;
    }

    memset(info, 0, sizeof(*info));

    T_I("Init(open, %s)",  item->name);
    if ((fd = open(item->fn, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
        T_E("Unable to create file: %s: %s", item->fn, strerror(errno));
        return ret;
    }

    T_I("Init(truncate, %s, " VPRIz ")",  item->name, item->len);
    if (truncate(item->fn, item->len) < 0) {
        T_E("Unable to adjust filesize of %s to " VPRIz " bytes: %s", item->fn, item->len, strerror(errno));
        goto do_exit;
    }

    T_I("Init(malloc, %s)",  item->name);
    if ((item->data = (vtss_flashaddr_t)VTSS_MALLOC(item->len)) == NULL) {
        T_E("Unable to allocate " VPRIz " bytes for data belonging to %s", item->len, item->fn);
        goto do_exit;
    }

    // Cache the current data from the file. The following should succeed,
    // since we've just adjusted the file size with truncate() above.
    T_I("Init(read, %s)",  item->name);
    if (read(fd, item->data, item->len) == item->len) {
        info->base_fladdr = item->data;
        info->size_bytes = item->len;
        ret = TRUE;
    } else {
        T_I("Init(read failed, %s)",  item->name);
    }

    if (!ret) {
        VTSS_FREE(item->data);
        item->data = NULL;
    }

    T_I("Init(exit, %s)",  item->name);

do_exit:
    close(fd);
    return ret;
}

static int file_flash_read(flash_section_t  *item,
                           vtss_flashaddr_t flash_addr,
                           void             *ram_addr,
                           size_t           len)
{
    T_I("RAM-based read(%s): %p to %p len " VPRIz,  item->name, flash_addr, ram_addr, len);

    if (item->data == NULL) {
        T_E("Flash file %s not initialized", item->fn);
        return VTSS_FLASH_ERR_GEN;
    }

    if (ram_addr == NULL || flash_addr == NULL) {
        T_E("Flash file (%s) should be read into non-NULL RAM (%p) and from non-NULL flash address (%p)", item->fn, ram_addr, flash_addr);
        return VTSS_FLASH_ERR_GEN;
    }

    // Check that the user only reads from addresses within our cached data
    if (flash_addr < item->data || flash_addr + len > item->data + item->len) {
        T_E("Flash file (%s): Reading from un-cached data area (req = " VPRIz " bytes from %p). Cache: " VPRIz " bytes from %p", item->fn, len, flash_addr, item->len, item->data);
        return VTSS_FLASH_ERR_GEN;
    }

    memcpy(ram_addr, flash_addr, len);

    return VTSS_FLASH_ERR_OK;
}

static int file_flash_erase(flash_section_t  *item,
                            vtss_flashaddr_t flash_addr,
                            size_t           len)
{
    T_I("RAM-based erase(%s): %p len " VPRIz, item->name, flash_addr, len);

    if (item->data == NULL) {
        T_E("Flash file %s not initialized", item->fn);
        return VTSS_FLASH_ERR_GEN;
    }

    if (flash_addr == NULL) {
        T_E("Flash erase(%s): flash_addr (%p) must be non-NULL", item->fn, flash_addr);
        return VTSS_FLASH_ERR_GEN;
    }

    // Check that the user only erases addresses within our cached data
    if (flash_addr < item->data || flash_addr + len > item->data + item->len) {
        T_E("Flash file (%s): Erasing un-cached data area (req = " VPRIz " bytes from %p). Cache: " VPRIz " bytes from %p", item->fn, len, flash_addr, item->len, item->data);
        return VTSS_FLASH_ERR_GEN;
    }

    memset(flash_addr, 0xff, len);

    return VTSS_FLASH_ERR_OK;
}

static int file_flash_program(flash_section_t  *item,
                              vtss_flashaddr_t flash_addr,
                              const void       *ram_addr,
                              size_t           len)
{
    int   rc  = VTSS_FLASH_ERR_GEN;
    off_t off = (flash_addr - item->data);
    int   fd;

    T_I("File program(%s): %p from %p len " VPRIz, item->name, flash_addr, ram_addr, len);

    if (item->data == NULL) {
        T_E("Flash file %s not initialized", item->fn);
        return rc;
    }

    if (flash_addr == NULL || ram_addr == NULL) {
        T_E("Flash program(%s): flash_addr (%p) and ram_addr(%p) must be non-NULL", item->fn, flash_addr, ram_addr);
        return rc;
    }

    // Check that the user only programs addresses within our cached data
    if (flash_addr < item->data || flash_addr + len > item->data + item->len) {
        T_E("Flash file (%s): Programming into un-cached data area (req = " VPRIz " bytes from %p). Cache: " VPRIz " bytes from %p", item->fn, len, flash_addr, item->len, item->data);
        return rc;
    }

    // Support writing directly from our cache into our cache (for the sake of file_flash_erase())
    if (flash_addr != ram_addr) {
        memcpy(flash_addr, ram_addr, len);
    }

    if ((fd = open(item->fn, O_RDWR)) < 0) {
        T_E("Unable to open %s for R/W", item->fn);
        return rc;
    }

    if (lseek(fd, off, SEEK_SET) == off) {
        if (write(fd, flash_addr, len) == len) {
            T_I("Write (%s) @ %p " VPRIz " bytes - offset %u", item->fn, (void *)flash_addr, len, (u32)off);
            rc = VTSS_FLASH_ERR_OK;
        } else {
            T_E("Write error (%s): %s", item->fn, strerror(errno));
        }
    } else {
        T_E("Seek error (%s):offset %u: %s", item->fn, (u32)off, strerror(errno));
    }

    close(fd);

    T_I("File program(%s): Exit", item->name);
    return rc;
}

struct flash_ops fops_file = {
    file_flash_init,
    file_flash_read,
    file_flash_program,
    file_flash_erase,
};

// Flash through MTD ---------------------

#if defined(USE_MTD_FOR_CONFIG)
static mesa_rc vtss_mtd_devopen(const char *name, int flags)
{
    FILE *procmtd;
    char mtdentry[128];
    int devno, fd;

    fd = -1;
    if ((procmtd = fopen("/proc/mtd", "r"))) {
        while (fgets(mtdentry, sizeof(mtdentry), procmtd)) {
            char *mtdname;
            if (sscanf(mtdentry, "mtd%d:", &devno) && (mtdname = strstr(mtdentry, " \"")) && strncmp(mtdname + 2, name, strlen(name)) == 0) {
                snprintf(mtdentry, sizeof(mtdentry), "/dev/mtd%d", devno);
                if ((fd = open(mtdentry, flags)) >= 0) {
                    T_I("Mtd open(%s) = %s", name, mtdentry);
                    break;
                }
            }
        }

        fclose(procmtd);
    }

    if (fd < 0) {
        VTSS_TRACE(ERROR) << "Mtd open(" << name << ") fails";
    }

    return fd;
}

static int mtd_flash_program(flash_section_t  *item,
                             vtss_flashaddr_t flash_addr,
                             const void       *ram_addr,
                             size_t           len)
{
    int rc = VTSS_FLASH_ERR_GEN;
    VTSS_TRACE(DEBUG) << "Mtd program(" << item->name << "): " << flash_addr << " from " << ram_addr << " len " << len;
    if (ram_addr == NULL || flash_addr == NULL) {
        return rc;
    }

    if (item) {
        off_t off = (flash_addr - item->data);
        int fd;

        if (flash_addr != ram_addr) {
            memcpy(flash_addr, ram_addr, len);
        }

        if ((fd = vtss_mtd_devopen(item->fn, O_SYNC | O_RDWR)) >= 0) {
            struct mtd_info_user mtd;
            if (off == 0 &&
                ioctl (fd, MEMGETINFO, &mtd) == 0) {
                int i;
                struct erase_info_user erase;
                for (i = 0; i < len; i += mtd.erasesize) {
                    erase.start = i;
                    erase.length = mtd.erasesize;
                    if (ioctl (fd, MEMERASE, &erase) < 0) {
                        VTSS_TRACE(ERROR) << "Erase error(" << item->name << "): " << erase.start << " len " << erase.length;
                    } else {
                        VTSS_TRACE(INFO) << "Erased(" << item->name << "): " << erase.start << " len " << erase.length;
                    }
                }
            }

            if (lseek(fd, off, SEEK_SET) == off) {
                if (write(fd, flash_addr, len) == len) {
                    VTSS_TRACE(DEBUG) << "Write @ " << (void *) flash_addr << " " << len << " bytes - offset " << (uint32_t) off;
                    rc = VTSS_RC_OK;
                } else {
                    VTSS_TRACE(ERROR) << "Write error: " << item->fn << " - " << strerror(errno);
                }
            } else {
                VTSS_TRACE(ERROR) << "Seek error: " << item->fn << " offset " << (uint32_t) off << " - " << strerror(errno);
            }

            close(fd);
        }
    }

    return rc;
}

static BOOL mtd_flash_init(flash_section_t *item, flash_mgmt_section_info_t *info)
{
    int fd;
    bool ret = FALSE;

    VTSS_TRACE(DEBUG) << "Mtd init(" << item->name << ", " << item->fn << ")";
    if ((fd = vtss_mtd_devopen(item->fn, O_SYNC | O_RDONLY)) >= 0) {
        struct mtd_info_user mtd;
        if (ioctl (fd, MEMGETINFO, &mtd) == 0) {
            item->data = (vtss_flashaddr_t)VTSS_MALLOC(item->len);
            if (item->data != NULL) {
                info->base_fladdr = item->data;
                info->size_bytes = item->len;
                ret = (item->len == read(fd, item->data, item->len));
            }
        }
        close(fd);
    } else {
        perror(item->fn);
    }

    return ret;
}

struct flash_ops fops_mtd = {
    mtd_flash_init,
    file_flash_read,
    mtd_flash_program,
    file_flash_erase,
};
#endif

// ----------------------------------

#define ENTRY_M(t,n,l) { .name = n, .fn = n                  , .len = l, .fop = t, }
#define ENTRY_F(t,n,l) { .name = n, .fn = VTSS_FS_FLASH_DIR n, .len = l, .fop = t, }
#define SIZEK(n) (n * 1024)

static flash_section_t flash_section[] = {
#if defined(USE_MTD_FOR_CONFIG)
    ENTRY_M(&fops_mtd,  "conf",      SIZEK(64)),
#else
    ENTRY_F(&fops_file, "conf",      SIZEK(64)),
#endif
    ENTRY_F(&fops_file, "stackconf", SIZEK(512)),
    ENTRY_F(&fops_file, "syslog",    SIZEK(64)),
};

static flash_section_t *_lookup_sec(const char *section_name)
{
    int i;
    for (i = 0; i < ARRSZ(flash_section); i++) {
        flash_section_t *item = &flash_section[i];
        if (strncmp(section_name, item->name, strlen(item->name)) == 0) {
            VTSS_TRACE(DEBUG) << "Lookup " << section_name << " OK, file " << item->fn;
            return item;
        }
    }
    VTSS_TRACE(INFO) << "Lookup " << section_name << " BAD";
    return NULL;
}

static flash_section_t *_lookup_ptr(vtss_flashaddr_t ptr, size_t len)
{
    int             i;
    flash_section_t *item;

    for (i = 0; i < ARRSZ(flash_section); i++) {
        item = &flash_section[i];

        if (item->data && ptr >= item->data && (ptr + len) <= (item->data + item->len)) {
            T_D("Lookup %p OK, sec %s", (void *)ptr, item->name);
            return item;
        }
    }

    T_W("Lookup %p failed", ptr);
    return NULL;
}

BOOL flash_mgmt_lookup(const char *section_name, flash_mgmt_section_info_t *info)
{
    BOOL ret = FALSE;

    flash_section_t *item = _lookup_sec(section_name);

    if (item) {
        ret = item->fop->init(item, info);
        VTSS_TRACE(DEBUG) << "Lookup " << section_name << (ret ? " OK " : " BAD ") << "base " << (void *)info->base_fladdr;
    }

    return ret;
}

/*---------------------------------------------------------------------------*/
/* Flash                                                                     */
int vtss_flash_read(const vtss_flashaddr_t flash_base,
                    void                   *ram_base,
                    size_t                 len,
                    vtss_flashaddr_t       *err_address)
{
    int rc = VTSS_FLASH_ERR_GEN;

    flash_section_t *item = _lookup_ptr(flash_base, len);

    if (item) {
        rc = item->fop->read(item, flash_base, ram_base, len);
    }

    VTSS_TRACE(DEBUG) << "Read @ " << (void *) flash_base << " " << len << " bytes, rc " << rc;
    if (rc != VTSS_RC_OK && err_address) {
        *err_address = flash_base;
    }

    return rc;
}

int vtss_flash_erase(vtss_flashaddr_t flash_base,
                     size_t           len,
                     vtss_flashaddr_t *err_address)
{
    int rc = VTSS_FLASH_ERR_GEN;

    flash_section_t *item = _lookup_ptr(flash_base, len);
    if (item) {
        rc = item->fop->erase(item, flash_base, len);
        VTSS_TRACE(DEBUG) << "Erase @ " << (void *) flash_base << " " << len << " bytes";
    }

    return rc;
}

int vtss_flash_program(vtss_flashaddr_t flash_base,
                       const void       *ram_base,
                       size_t           len,
                       vtss_flashaddr_t *err_address)
{
    int rc = VTSS_FLASH_ERR_GEN;

    flash_section_t *item = _lookup_ptr(flash_base, len);
    if (item) {
        rc = item->fop->program(item, flash_base, ram_base, len);
    }

    if (rc != VTSS_RC_OK && err_address) {
        *err_address = flash_base;
    }

    return rc;
}

const char *vtss_flash_errmsg(const int err)
{
    switch (err) {
    case VTSS_FLASH_ERR_OK:
        return "Flash OK";

    case VTSS_FLASH_ERR_GEN:
        return "General Error";

    default:
        return "Unknown Error";
    }
}


/*---------------------------------------------------------------------------*/
/* CRC                                                                       */

u32 vtss_crc32(const unsigned char *s, int len)
{
    return crc32(0, s, len);
}

u32 vtss_crc32_accumulate(u32 crc, const unsigned char *s, int len)
{
    return crc32(crc, s, len);
}

static mesa_rc vtss_httpd_base64_error_handling(i32 error_code)
{
    if (error_code == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) {
        VTSS_TRACE(DEBUG) << "Output buffer too small.";
        return VTSS_RC_ERROR;
    }

    if (error_code == MBEDTLS_ERR_BASE64_INVALID_CHARACTER) {
        VTSS_TRACE(DEBUG) << "Invalid character in input.";
        return VTSS_RC_ERROR;
    }

    if (error_code != 0) {
        VTSS_TRACE(ERROR) << "Unknown error:" << error_code;
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_httpd_base64_encode(char *to, size_t to_len, const char *from, size_t from_len)
{
    if (to == NULL || from == NULL) {
        VTSS_TRACE(ERROR) << "'to' and 'from' can't be NULL";
        return VTSS_RC_ERROR;
    }

    VTSS_RC(vtss_httpd_base64_error_handling(mbedtls_base64_encode((unsigned char *) to, to_len, &to_len, (unsigned char *) from, from_len)));
    to[to_len] = 0; /* mbedtls_base64_encode() sets the ending '\0' before success return, but we add it for insurance. */

    return VTSS_RC_OK;
}

// Decoding a base64 encoded string.
// INOUT : in_out - Contains the encoded string. Returned as plain text.
mesa_rc std_string_base64_decode(std::string &in_out)
{
    // TODO - consider making this work inplace!
    size_t res_len;
    std::string out_txt;

    // Reserve space in out_txt - decoded result is always shorter than encoded
    out_txt.resize(in_out.size());
    // Do the decoding - Note - MUST return on error (handled by VTSS_RC)
    VTSS_RC(vtss_httpd_base64_error_handling(mbedtls_base64_decode((unsigned char *) out_txt.c_str(),
                                                                   out_txt.size(),
                                                                   &res_len,
                                                                   (unsigned char *)in_out.c_str(), in_out.size())));

    // Adjust the size of the resulting buffer
    out_txt.resize(res_len);

    // Swap the content
    std::swap(out_txt, in_out);

    return VTSS_RC_OK;
}

mesa_rc vtss_httpd_base64_decode(char *to, size_t to_len, const char *from, size_t from_len)
{
    if (to == NULL || from == NULL) {
        VTSS_TRACE(ERROR) << "'to' and 'from' can't be NULL";
        return VTSS_RC_ERROR;
    }

    VTSS_RC(vtss_httpd_base64_error_handling(mbedtls_base64_decode((unsigned char *)to, to_len - 1, &to_len, (unsigned char *)from, from_len)));
    to[to_len] = 0; /* mbedtls_base64_decode() doesn't set the ending '\0' before success return. */

    return VTSS_RC_OK;
}

// Calculate hash value with SHA256.
// In     : input_str     - The input string.
// In     : input_str_len - The length of input string.
// Out    : output_bytes  - The output of hash value (hex bytes).
// Return : VTSS_RC_OK if the process was done correct, else error code.
// leftrotate function definition
mesa_rc vtss_sha1_calc(u8 *input_str, u32 input_str_len, u8 output_bytes[20])
{
    mbedtls_sha1_context ctx;

    mbedtls_sha1_init(&ctx);
    mbedtls_sha1_starts(&ctx);
    mbedtls_sha1_update(&ctx, input_str, input_str_len);
    mbedtls_sha1_finish(&ctx, output_bytes);
    mbedtls_sha1_free(&ctx);

    return VTSS_RC_OK;
}

// Calculate hash value with SHA256.
// In     : input_str     - The input string.
// In     : input_str_len - The length of input string.
// Out    : output_bytes  - The output of hash value (hex bytes).
// Return : VTSS_RC_OK if the process was done correct, else error code.
mesa_rc vtss_sha256_calc(u8 *input_str, u32 input_str_len, u8 output_bytes[32])
{
    mbedtls_sha256_context ctx;

    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, FALSE);
    mbedtls_sha256_update(&ctx, input_str, input_str_len);
    mbedtls_sha256_finish(&ctx, output_bytes);
    mbedtls_sha256_free(&ctx);

    return VTSS_RC_OK;
}

// Generate a random hex bytes.
// In     : byte_num - The number of hex byte.
// Out    : hex_bytes - The output of random hex bytes.
// Return : VTSS_RC_OK if the process was done correct, else error code.
static mesa_rc vtss_generate_random_hex_bytes(u32 byte_num, u8 *hex_bytes)
{
    int fd = 0;
    ssize_t len = 0;

    fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        return VTSS_RC_ERROR;
    }

    while (len < byte_num) {
        ssize_t tmp = read(fd, hex_bytes + len, byte_num - len);
        if (tmp < 0) {
            close(fd);
            return VTSS_RC_ERROR;
        }
        len += tmp;
    }

    close(fd);
    return VTSS_RC_OK;
}

// Generate a random hex string.
// In     : byte_num - The number of hex byte.
// Out    : output  - The output of random hex string.
//          Notice the output buffer size should great than (2 * byte_num + 1)
// Return : VTSS_RC_OK if the process was done correct, else error code.
mesa_rc vtss_generate_random_hex_str(u32 byte_num, char *output)
{
    u8 hex_bytes[byte_num];
    int ret, idx;

    ret = vtss_generate_random_hex_bytes(byte_num, hex_bytes);

    if (ret == 0) {
        // Convert hash hex value to string
        for (idx = 0; idx < byte_num; idx++) {
            sprintf(&output[idx * 2], "%02x", (u8)hex_bytes[idx]);
        }
        output[byte_num * 2] = '\0';
    }

    return ret == 0 ? VTSS_RC_OK : VTSS_RC_ERROR;
}

// Encrypt a clear text with AES256.
// In     : input_str     - The input string.
// In     : key           - The key for encryption.
// In     : key_size      - The key size (bytes).
// In     : output_str_buf_len - The buffer length for output text.
// Out    : output_str    - The output of encrypted value (hex bytes).
// Return : VTSS_RC_OK if the process was done correct, else error code.
mesa_rc vtss_aes256_encrypt(char *input_str, u8 *key, u32 key_size, u32 output_str_buf_len, char *output_str)
{
    mesa_rc                 rc = VTSS_RC_ERROR;
    mbedtls_aes_context     aes_ctx;
    mbedtls_md_context_t    sha_ctx;
    int                     idx, n, lastn;
    unsigned char           IV[16];
    unsigned char           digest[32];
    unsigned char           buffer[1024];
    unsigned char           output_bytes[128];
    size_t                  input_len = strlen(input_str);
    size_t                  output_bytes_len = 0;
    off_t                   offset;

    /* Check the input parameters */
    if (output_str_buf_len < 96) {
        // Min. output string length is 96.
        return rc;
    }

    /* Initalizate resource */
    mbedtls_aes_init(&aes_ctx);
    mbedtls_md_init(&sha_ctx);
    if (mbedtls_md_setup(&sha_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1) != 0) {
        goto exit_encrypt;
    }
    memset(IV, 0, sizeof(IV));
    memset(digest, 0, sizeof(digest));
    memset(buffer, 0, sizeof(buffer));
    memset(output_str, 0, output_str_buf_len);

    /* Generate the initialization vector. */
    // Generate a random 16 bytes as the IV.
    // It makes the different result even if the same plain text.
    if (vtss_generate_random_hex_bytes(sizeof(IV), IV) != VTSS_RC_OK) {
        goto exit_encrypt;
    }

    /* The last four bits in the IV are actually used
     * to store the file size modulo the AES block size. */
    lastn = (int)(input_len & 0x0F);
    IV[15] = (unsigned char)((IV[15] & 0xF0) | lastn);

    /* Write the IV at the beginning of the output. */
    memcpy(&output_bytes[output_bytes_len], IV, sizeof(IV));
    output_bytes_len += sizeof(IV);

    /* Hash the IV and the secret key together 8192 times
     * using the result to setup the AES context and HMAC. */
    memcpy(digest, IV, 16);
    for (idx = 0; idx < 8192; idx++) {
        mbedtls_md_starts(&sha_ctx);
        mbedtls_md_update(&sha_ctx, digest, 32);
        mbedtls_md_update(&sha_ctx, key, key_size);
        mbedtls_md_finish(&sha_ctx, digest);
    }
    mbedtls_aes_setkey_enc(&aes_ctx, digest, 256);
    mbedtls_md_hmac_starts(&sha_ctx, digest, 32);

    /* Encrypt and write the ciphertext. */
    for (offset = 0; offset < input_len; offset += 16) {
        // Read input string to buffer
        n = (input_len - offset > 16) ? 16 : (int) (input_len - offset);
        memcpy(buffer, (u8 *) &input_str[offset], n);

        for (idx = 0; idx < 16; idx++) {
            buffer[idx] = (unsigned char)(buffer[idx] ^ IV[idx]);
        }

        mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, buffer, buffer);
        mbedtls_md_hmac_update(&sha_ctx, buffer, 16);

        // Write to output bytes
        memcpy(&output_bytes[output_bytes_len], buffer, 16);
        output_bytes_len += 16;
        memcpy(IV, buffer, 16);
    }

    /* Write the HMAC  at the ending of the output. */
    mbedtls_md_hmac_finish(&sha_ctx, digest);
    memcpy(&output_bytes[output_bytes_len], digest, sizeof(digest));
    output_bytes_len += sizeof(digest);

    /* Convert the output bytes to hex string */
    for (idx = 0; idx < output_bytes_len; idx++) {
        sprintf(&output_str[idx * 2], "%02x", (u8)output_bytes[idx]);
    }
    rc = VTSS_RC_OK;

exit_encrypt:
    mbedtls_aes_free(&aes_ctx);
    mbedtls_md_free(&sha_ctx);

    return rc;
}

// Decrypt an AES256 hex value.
// In     : input_hex_str      - The encrypted hex string.
// In     : key                - The key for decryption.
// In     : key_size           - The key size (bytes).
// In     : output_str_buf_len - The buffer length for output text.
// Out    : output_str         - The output text after decryption.
// Return : VTSS_RC_OK if the process was done correct, else error code.
mesa_rc vtss_aes256_decrypt(char input_hex_str[256], u8 *key, u32 key_size, u32 output_str_buf_len, char *output_str)
{
    mesa_rc                 rc = VTSS_RC_ERROR;
    mbedtls_aes_context     aes_ctx;
    mbedtls_md_context_t    sha_ctx;
    unsigned char           IV[16];
    unsigned char           digest[32];
    unsigned char           buffer[1024];
    unsigned char           input_bytes[128];
    int                     idx, n, lastn;
    unsigned char           tmp[16];
    off_t                   offset;
    size_t                  input_len = strlen(input_hex_str);

    // Check input string length
    if (input_len < 48 * 2) {
        // The encrypted string should be structured as follows:
        //       00 .. 15              Initialization Vector
        //       16 .. 31              AES Encrypted Block #1
        //          ..
        //     N*16 .. (N+1)*16 - 1    AES Encrypted Block #N
        // (N+1)*16 .. (N+1)*16 + 32   HMAC-SHA-256(ciphertext)

        // Input hex string is too short to be encrypted.
        return rc;
    }
    if ((input_len & 0x0F) != 0) {
        // Input hex string length is not a multiple of 16.
        return rc;
    }

    // Check input string format (must hex characters) and convert to hex
    for (idx = 0; idx < input_len; idx++) {
        const char *c = &input_hex_str[idx];

        if (!((*c >= '0' && *c <= '9') || (*c >= 'A' && *c <= 'F') || (*c >= 'a' && *c <= 'f'))) {
            // Input string is not hex characters.
            return rc;
        }
        if ((idx + 1) % 2) {
            if (sscanf(c, "%02x", &n) <= 0) {
                return rc;
            }
            input_bytes[idx / 2] = (unsigned char) n;
        }
    }

    /* Initalizate resource */
    mbedtls_aes_init(&aes_ctx);
    mbedtls_md_init(&sha_ctx);
    if (mbedtls_md_setup(&sha_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1) != 0) {
        goto exit_decrypt;
    }
    memset(IV, 0, sizeof(IV));
    memset(digest, 0, sizeof(digest));
    memset(buffer, 0, sizeof(buffer));
    memset(output_str, 0, output_str_buf_len);

    /* Read the IV and original size modulo 16. */
    memcpy(buffer, input_bytes, sizeof(IV));
    memcpy(IV, buffer, sizeof(IV));
    lastn = IV[15] & 0x0F;

    /* Hash the IV and the secret key together 8192 times
     * using the result to setup the AES context and HMAC. */
    memset(digest, 0, sizeof(digest));
    memcpy(digest, IV, 16);

    for (idx = 0; idx < 8192; idx++) {
        mbedtls_md_starts(&sha_ctx);
        mbedtls_md_update(&sha_ctx, digest, sizeof(digest));
        mbedtls_md_update(&sha_ctx, key, key_size);
        mbedtls_md_finish(&sha_ctx, digest);
    }

    mbedtls_aes_setkey_dec(&aes_ctx, digest, 256);
    mbedtls_md_hmac_starts(&sha_ctx, digest, sizeof(digest));

    /* Decrypt and write the plaintext. */
    input_len /= 2; // Converet to byte length
    input_len -= sizeof(IV); // Subtract the IV length (16)
    input_len -= sizeof(digest); // Subtract the HMAC length (32)
    for (offset = 0; offset < input_len; offset += 16) {
        memcpy(buffer, &input_bytes[offset + sizeof(IV)], 16);
        memcpy(tmp, buffer, 16);

        mbedtls_md_hmac_update(&sha_ctx, buffer, 16);
        mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_DECRYPT, buffer, buffer);

        for (idx = 0; idx < 16; idx++) {
            buffer[idx] = (unsigned char)(buffer[idx] ^ IV[idx]);
        }

        memcpy(IV, tmp, 16);

        n = (lastn > 0 && offset == input_len - 16) ? lastn : 16;

        if ((offset + n) >= output_str_buf_len) {
            // Output string buffer too small
            rc = VTSS_RC_ERROR;
            goto exit_decrypt;
        } else {
            memcpy(&output_str[offset], buffer, n);
            output_str[offset + n] = '\0';
        }
    }

    /* Verify the message authentication code. */
    mbedtls_md_hmac_finish(&sha_ctx, digest);
    if (!memcmp(&input_bytes[input_len + sizeof(IV)], digest, sizeof(digest))) {
        rc = VTSS_RC_OK;
    }

exit_decrypt:
    mbedtls_aes_free(&aes_ctx);
    mbedtls_md_free(&sha_ctx);

    return rc;
}

/*---------------------------------------------------------------------------*/
/* Serial IO                dummy implementations                            */

// Set TTY raw mode, 115K 8-1-N
static mesa_rc vtss_io_tty_setup(int fd)
{
    struct termios buf;

    if (tcgetattr(fd, &buf) < 0) {
        /* get the original state */
        VTSS_TRACE(ERROR) << "tcgetattr failed";
        return VTSS_RC_ERROR;
    }

    // Setup mode
    cfmakeraw(&buf);
    cfsetspeed(&buf, B115200);

    if (tcsetattr(fd, TCSANOW, &buf) < 0) {
        VTSS_TRACE(ERROR) << "tty setup failed";
        return VTSS_RC_ERROR;
    }

    VTSS_TRACE(DEBUG) << "tty setup OK";
    return VTSS_RC_OK;
}

// Lookup a device and return it's handle
mesa_rc vtss_io_lookup(const char *name, vtss_io_handle_t *handle)
{
    int     fd = open(name, O_RDWR);
    mesa_rc rc;

    if (fd >= 0) {
        VTSS_TRACE(DEBUG) << "serial device " << name << " opened";
        *handle = fd;
        if ((rc = vtss_io_tty_setup(fd)) != VTSS_RC_OK) {
            close(fd);
        }
    } else {
        rc = VTSS_RC_ERROR;
        VTSS_TRACE(ERROR) << "serial device " << name << " fails to open";
    }

    return rc;
}

// Write data to a device
mesa_rc vtss_io_write(vtss_io_handle_t handle, const void *buf, size_t *len)
{
    mesa_rc rc;
    ssize_t nwritten = write(handle, buf, *len);
    if (nwritten != *len) {
        rc = VTSS_RC_ERROR;
        *len = nwritten;
        VTSS_TRACE(ERROR) << "vtss_io_write failed, request " << *len << " wrote " << nwritten;
    } else {
        rc = VTSS_RC_OK;
    }
    return rc;
}

// Read data from a device
mesa_rc vtss_io_read(vtss_io_handle_t handle, void *buf, size_t *len)
{
    mesa_rc rc;
    ssize_t nread = read(handle, buf, *len);
    if (nread < 0) {
        VTSS_TRACE(ERROR) << "vtss_io_read failed";
        rc = VTSS_RC_ERROR;
    } else {
        *len = nread;
        rc = VTSS_RC_OK;
    }
    return rc;
}

// Set config for device
mesa_rc vtss_io_set_config(vtss_io_handle_t handle, u32 key, const void *buf, size_t *len)
{
    if (key == VTSS_IO_SET_CONFIG_SERIAL_INFO) {
        struct termios termios_buf;
        if (tcgetattr(handle, &termios_buf) < 0) {
            VTSS_TRACE(ERROR) << "tcgetattr failed";
            return VTSS_RC_ERROR;
        }

        vtss_serial_info_t serial_info;
        memcpy(&serial_info, buf, sizeof(vtss_serial_info_t));

        switch (serial_info.baud) {
        case VTSS_SERIAL_BAUD_9600:
            cfsetspeed(&termios_buf, B9600);
            break;
        case VTSS_SERIAL_BAUD_19200:
            cfsetspeed(&termios_buf, B19200);
            break;
        case VTSS_SERIAL_BAUD_38400:
            cfsetspeed(&termios_buf, B38400);
            break;
        case VTSS_SERIAL_BAUD_115200:
            cfsetspeed(&termios_buf, B115200);
            break;
        default:
            VTSS_TRACE(WARNING) << "unsupported baudrate";
            cfsetspeed(&termios_buf, B115200);
        }

        switch (serial_info.parity) {
        case VTSS_SERIAL_PARITY_ODD:
            termios_buf.c_cflag |= (PARENB | PARODD);
            break;
        case VTSS_SERIAL_PARITY_EVEN:
            termios_buf.c_cflag = (termios_buf.c_cflag | PARENB) & ~PARODD;
            break;
        default:
            termios_buf.c_cflag &= ~(PARENB | PARODD);
            break;
        }

        switch (serial_info.word_length) {
        case VTSS_SERIAL_WORD_LENGTH_5:
            termios_buf.c_cflag = (termios_buf.c_cflag & ~CSIZE) | CS5;
            break;
        case VTSS_SERIAL_WORD_LENGTH_6:
            termios_buf.c_cflag = (termios_buf.c_cflag & ~CSIZE) | CS6;
            break;
        case VTSS_SERIAL_WORD_LENGTH_7:
            termios_buf.c_cflag = (termios_buf.c_cflag & ~CSIZE) | CS7;
            break;
        case VTSS_SERIAL_WORD_LENGTH_8:
            termios_buf.c_cflag = (termios_buf.c_cflag & ~CSIZE) | CS8;
            break;
        default:
            VTSS_TRACE(WARNING) << "unsupported word length";
            termios_buf.c_cflag = (termios_buf.c_cflag & ~CSIZE) | CS8;
        }

        if (serial_info.stop == VTSS_SERIAL_STOP_2) {
            termios_buf.c_cflag |= CSTOPB;
        } else {
            termios_buf.c_cflag &= ~CSTOPB;
        }

        if ((serial_info.flags & VTSS_SERIAL_FLOW_RTSCTS_RX) && (serial_info.flags & VTSS_SERIAL_FLOW_RTSCTS_TX)) {
            termios_buf.c_cflag |= CRTSCTS;
        } else {
            termios_buf.c_cflag &= ~CRTSCTS;
        }

        if (tcsetattr(handle, TCSANOW, &termios_buf) < 0) {
            VTSS_TRACE(ERROR) << "tcsetattr failed";
            return VTSS_RC_ERROR;
        }
        *len = sizeof(vtss_serial_info_t);
        return VTSS_RC_OK;
    } else {
        VTSS_TRACE(WARNING) << "vtss_io_set_config sub-function not yet implemented on Linux";
        return VTSS_RC_ERROR;
    }
}

// Read config for device
mesa_rc vtss_io_get_config(vtss_io_handle_t handle, u32 key, void *buf, size_t *len)
{
    if (key == VTSS_IO_GET_CONFIG_SERIAL_INFO) {
        struct termios termios_buf;
        if (tcgetattr(handle, &termios_buf) < 0) {
            VTSS_TRACE(ERROR) << "tcgetattr failed";
            return VTSS_RC_ERROR;
        }

        vtss_serial_info_t serial_info;
        speed_t o_baud = cfgetospeed(&termios_buf);
        speed_t i_baud = cfgetispeed(&termios_buf);
        if (o_baud != i_baud) {
            VTSS_TRACE(WARNING) << "input and output baudrate configured to different values";
        }
        switch (o_baud) {
        case B9600:
            serial_info.baud = VTSS_SERIAL_BAUD_9600;
            break;
        case B19200:
            serial_info.baud = VTSS_SERIAL_BAUD_19200;
            break;
        case B38400:
            serial_info.baud = VTSS_SERIAL_BAUD_38400;
            break;
        case B115200:
            serial_info.baud = VTSS_SERIAL_BAUD_115200;
            break;
        default:
            VTSS_TRACE(WARNING) << "unsupported baudrate";
            serial_info.baud = VTSS_SERIAL_BAUD_115200;
        }

        if (termios_buf.c_cflag & PARENB) {
            if (termios_buf.c_cflag & PARODD) {
                serial_info.parity = VTSS_SERIAL_PARITY_ODD;
            } else {
                serial_info.parity = VTSS_SERIAL_PARITY_EVEN;
            }
        } else {
            serial_info.parity = VTSS_SERIAL_PARITY_NONE;
        }

        switch (termios_buf.c_cflag & CSIZE) {
        case CS5:
            serial_info.word_length = VTSS_SERIAL_WORD_LENGTH_5;
            break;
        case CS6:
            serial_info.word_length = VTSS_SERIAL_WORD_LENGTH_6;
            break;
        case CS7:
            serial_info.word_length = VTSS_SERIAL_WORD_LENGTH_7;
            break;
        case CS8:
            serial_info.word_length = VTSS_SERIAL_WORD_LENGTH_8;
            break;
        default:
            VTSS_TRACE(WARNING) << "unsupported word size";
            serial_info.word_length = VTSS_SERIAL_WORD_LENGTH_8;
        }

        if (termios_buf.c_cflag & CSTOPB) {
            serial_info.stop = VTSS_SERIAL_STOP_2;
        } else {
            serial_info.stop = VTSS_SERIAL_STOP_1;
        }

        if (termios_buf.c_cflag & CRTSCTS) {
            serial_info.flags = VTSS_SERIAL_FLOW_RTSCTS_RX | VTSS_SERIAL_FLOW_RTSCTS_TX;
        } else  {
            serial_info.flags = 0;
        }

        memcpy(buf, &serial_info, sizeof(vtss_serial_info_t));
        *len = sizeof(vtss_serial_info_t);
        return VTSS_RC_OK;
    } else {
        VTSS_TRACE(WARNING) << "vtss_io_get_config sub-function not yet implemented on Linux";
        return VTSS_RC_ERROR;
    }
}

// Generate random number
mesa_rc vtss_random(uint32_t *p)
{
    static int init = 0;
    const char *dev = "/dev/urandom"; // This is non-blocking. /dev/random may block, but is more secure.

    if (!init) {
        unsigned int seed;
        int fd_rand = open(dev, O_RDONLY);
        if (fd_rand < 0) {
            T_E("open(%s) failed: %s", dev, strerror(errno));
            return VTSS_RC_ERROR;
        }

        // read new seed
        if (read(fd_rand, (void *)&seed, sizeof(seed)) == sizeof(seed)) {
            srand(seed); // set new seed
            init = 1;
        } else {
            T_E("read(%s) failed: %s", dev, strerror(errno));
        }

        close(fd_rand);
    }

    if (!init) {
        return VTSS_RC_ERROR;
    }

    *p = rand();
    return VTSS_RC_OK;
}
