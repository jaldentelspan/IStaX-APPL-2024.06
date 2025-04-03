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
#include "lock.hxx"
#include "main.h"

#undef  VTSS_ALLOC_MODULE_ID
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_MISC

namespace vtss {

struct Lock_impl {
    bool locked_;
    vtss_cond_t cond_;
    vtss_mutex_t mutex_;
};

//Lock::Lock(bool b) : impl_( new Lock_impl(b)) {
Lock::Lock(bool b)  {
    impl_ = (Lock_impl*)VTSS_MALLOC(sizeof(Lock_impl));
    VTSS_ASSERT(impl_ != nullptr);
    impl_->locked_ = b;
    vtss_mutex_init(&impl_->mutex_);
    vtss_cond_init(&impl_->cond_, &impl_->mutex_);
}

Lock::~Lock() {
    VTSS_FREE(impl_);
}
    
// lock or unlock Lock
void Lock::lock(bool b) {
    vtss_mutex_lock(&impl_->mutex_);
    impl_->locked_ = b;
    if (!impl_->locked_) {
        vtss_cond_broadcast(&impl_->cond_);
    }

    vtss_mutex_unlock(&impl_->mutex_);
}

// Wait for Lock to be unlocked
void Lock::wait() {
    vtss_mutex_lock(&impl_->mutex_);
    while (impl_->locked_) {
        vtss_cond_wait(&impl_->cond_);
    }

    vtss_mutex_unlock(&impl_->mutex_);
}

void Lock::timed_wait(vtss_tick_count_t abstime) {
    vtss_mutex_lock(&impl_->mutex_);
    while (impl_->locked_) {
        (void)vtss_cond_timed_wait(&impl_->cond_, abstime);
        if (vtss_current_time() >= abstime) {
            impl_->locked_ = false;
        }
    }

    vtss_mutex_unlock(&impl_->mutex_);
}

} // namespace vtss

