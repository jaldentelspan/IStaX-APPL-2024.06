/* -*- Mode: C; c-basic-offset: 2; tab-width: 8; c-comment-only-line-offset: 0; -*- */
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

#ifndef _BOARD_SUBJECTS_H_
#define _BOARD_SUBJECTS_H_

#include <vtss/basics/notifications.hxx>
#include <vtss/basics/array.hxx>

// Allowed sequential irq faults before disabling the IRQ.
#define ALLOWED_IRQ_FAULTS 5

namespace vtss {
namespace notifications {
extern SubjectRunner subject_irq_thread;
}  // notifications

struct BoardIrq {

  constexpr BoardIrq() = delete;

  constexpr explicit BoardIrq(mesa_irq_t i, const char *n) noexcept :
      irqno_(i),
      name_(n) {}

  const mesa_irq_t irq_no() {return irqno_;}
  const char *name() {return name_;}
  const u32  cnt() {return cnt_;}     // Number of events on this IRQ
  const void tally() {cnt_++;}        // Increase events on this IRQ

private:
  const mesa_irq_t irqno_;
  const char       *name_;
  u32              cnt_ = 0;
};

// List of all IRQs.
extern vtss::Array<BoardIrq*, MESA_IRQ_MAX> irq_list;

void board_subjects_start(int irq_fd, const char *uio);
} // namespace vtss

// Allow iteration on mesa_irq_t enum
VTSS_ENUM_INC(mesa_irq_t);

#endif /* _BOARD_SUBJECTS_H_ */

