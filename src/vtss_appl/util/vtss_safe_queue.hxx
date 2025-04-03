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
#ifndef __VTSS_SAFE_QUEUE_H__
#define __VTSS_SAFE_QUEUE_H__

#include "main.h"
#include <vtss/basics/vector.hxx>
#include <vtss_module_id.h>

namespace vtss{
struct SafeQueue
{
    SafeQueue(size_t s = 10) : queue(s) {
    vtss_mutex_init(&mutex_);
    vtss_cond_init(&cond_, &mutex_);
    }

    ~SafeQueue() {
    }

    Vector<void*> queue;

    vtss_cond_t cond_;

    vtss_mutex_t mutex_;

    void *vtss_safe_queue_get();

    void *vtss_safe_queue_tryget();

    void *vtss_safe_queue_timed_get(vtss_tick_count_t abstime);

    vtss_bool_t vtss_safe_queue_put(void *item);
};
} /* namespace vtss */

#endif /* __VTSS_SAFE_QUEUE_H__ */
