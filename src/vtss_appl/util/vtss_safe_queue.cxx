/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "vtss_safe_queue.hxx"
#include "critd_api.h"
#include <vtss_trace_lvl_api.h>
#include <vtss_trace_api.h>
#include "vtss/basics/trace.hxx"

namespace vtss {
namespace appl {
namespace safe_queue {

struct Lock {
    Lock(vtss_mutex_t *mutex, const char *file, unsigned int line): mutex_(mutex), file_(file), line_(line) {
        vtss_mutex_lock(mutex_);
    }
    ~Lock() {
        vtss_mutex_unlock(mutex_);
    }
    vtss_mutex_t *mutex_;
    const char *file_;
    unsigned int line_;
};

}  // namespace safe_queue
}  // namespace appl

/*---------------------------------------------------------------------------*/
/* Safe queue to replace mbox                                                */

void *SafeQueue::vtss_safe_queue_get()
{
    void            *buff = NULL;

    appl::safe_queue::Lock l(&mutex_, __FILE__, __LINE__);
    while (queue.empty()) {
        vtss_cond_wait(&cond_);
    }
    buff = queue[0];
    queue.erase(queue.begin());
    vtss_cond_broadcast(&cond_);
    return buff;
}

void *SafeQueue::vtss_safe_queue_tryget()
{
    void *buff = NULL;

    appl::safe_queue::Lock l(&mutex_, __FILE__, __LINE__);
    if (queue.empty()) {
        return NULL;
    }

    buff = queue[0];
    queue.erase(queue.begin());
    vtss_cond_broadcast(&cond_);
    return buff;
}

void *SafeQueue::vtss_safe_queue_timed_get(vtss_tick_count_t abstime)
{
    void           *buff = NULL;

    appl::safe_queue::Lock l(&mutex_, __FILE__, __LINE__);
    while (queue.empty()) {
        int ret = vtss_cond_timed_wait(&cond_, abstime);
        if (!ret) {
            return NULL;
        }
    }
    buff = queue[0];
    queue.erase(queue.begin());
    vtss_cond_broadcast(&cond_);
    return buff;
}

vtss_bool_t SafeQueue::vtss_safe_queue_put(void *item)
{
    appl::safe_queue::Lock l(&mutex_, __FILE__, __LINE__);
    while (queue.size() >= queue.capacity()) {
        vtss_cond_wait(&cond_);
    }
    queue.push_back(item);

    vtss_cond_broadcast(&cond_);
    return true;
}

}  // namespace vtss
