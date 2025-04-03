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

#ifndef __VTSS_BASICS_NOTIFICATIONS_STRUCT_CONFIG_HXX__
#define __VTSS_BASICS_NOTIFICATIONS_STRUCT_CONFIG_HXX__

#include <vtss/basics/expose/param-tuple-val.hxx>
#include <vtss/basics/notifications/subject-base.hxx>
#include <vtss/basics/notifications/lock-global-subject.hxx>

namespace vtss {
namespace expose {

template <typename... Args>
struct StructConfig : public notifications::SubjectBase {
    mesa_rc get(typename Args::get_ptr_type... args) const {
        notifications::LockGlobalSubject lock(__FILE__, __LINE__);
        tuple_.get(args...);

        return MESA_RC_OK;
    }

    mesa_rc get(notifications::Event &ev, typename Args::get_ptr_type... args) {
        notifications::LockGlobalSubject lock(__FILE__, __LINE__);
        attach_(ev);
        tuple_.get(args...);

        return MESA_RC_OK;
    }

    mesa_rc set(typename Args::set_ptr_type... args) {
        // Calling the "direct" handler must be done before locking
        mesa_rc rc = set_impl(args...);
        if (rc != MESA_RC_OK) return rc;

        notifications::LockGlobalSubject lock(__FILE__, __LINE__);
        if (!tuple_.equal(args...)) {
            tuple_.set(args...);
            signal();
        }

        return MESA_RC_OK;
    }

  protected:
    // mutable because pre_get must be const
    mutable ParamTupleVal<Args...> tuple_;

  private:
    virtual mesa_rc set_impl(typename Args::set_ptr_type... args) = 0;
};

}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_NOTIFICATIONS_STRUCT_CONFIG_HXX__
