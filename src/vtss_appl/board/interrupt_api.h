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

#ifndef _VTSS_INTERRUPT_API_H_
#define _VTSS_INTERRUPT_API_H_

#include "vtss/basics/enum_macros.hxx"
#include "vtss/basics/vector.hxx"

#ifdef __cplusplus
extern "C" {
#endif

#include "main_types.h"
#include "main_types.h"
#include <vtss/appl/module_id.h>
#include <microchip/ethernet/board/api.h>

typedef enum {
    INTERRUPT_PRIORITY_NORMAL,
    INTERRUPT_PRIORITY_CLOCK,
    INTERRUPT_PRIORITY_POE,
    INTERRUPT_PRIORITY_PROTECT,
    INTERRUPT_PRIORITY_HIGHEST,
    INTERRUPT_PRIORITY_LAST      // No priority after this
} vtss_interrupt_priority_t;

typedef void (*vtss_interrupt_function_hook_t)(meba_event_t source_id, u32 instance_id);

/**
 * Assign a function_hook to a source instance - in a device.
 *
 * When hook is assigned function_hook is inserted in a prioritized list and the
 * interrupt on source is enabled. When an interrupt occurs on the source, the
 * interrupt is disabled and all function_hook are removed from list and called
 * in a prioritized order. Always re-assign hook before getting the actual
 * status on the source (in order not to miss an interrupt). Remember that
 * whatever code excecuted in this function_hook is excecuted on interrupt
 * thread priority.
 *
 * The code in the function_hook could be merely registration of interrupt and
 * signal of a semaphore to activate own thread.
 *
 * \param function_hook [IN] Callback function pointer.
 * \param source_id     [IN] Identification of source.
 * \param instance_id   [IN] Identification of instance.
 * \param priority      [IN] Call back priority.
 *
 * \return : Return code. Since this is an application-internal API, a trace error
 * is thrown in case of errors.
 */
mesa_rc vtss_interrupt_source_hook_set(vtss_module_id_t module_id, vtss_interrupt_function_hook_t function_hook, meba_event_t source_id, vtss_interrupt_priority_t priority);

/**
 * Remove a function hook previously assigned with vtss_interrupt_source_hook_set()
 *
 * \param function_hook [IN] Callback function pointer.
 * \param source_id     [IN] Identification of source.
 *
 * \return : Return code. Since this is an application-internal API, a trace error
 * is thrown in case of errors.
 */
mesa_rc vtss_interrupt_source_hook_clear(vtss_interrupt_function_hook_t function_hook, meba_event_t source_id);

/**
 * Convert an interrupt source to a textual representation.
 *
 * \param source_id [IN] Identification of source.
 *
 * \return : Return code. Since this is an application-internal API, a trace error
 * is thrown in case of errors.
 */
const char *vtss_interrupt_source_text(meba_event_t source_id);

/**
 * Get info about a particular interrupt source ID.
 *
 * The function returns the number of times the underlying IRQ has caused
 * this source (application) interrupt to fire. It returns the number
 * of times it has been invoked with and without listeners on the source ID
 * (where a listener is a function installed with vtss_interrupt_source_hook_set()).
 *
 * \param source_id   [IN]  Identification of source.
 * \param hook_cnt    [OUT] Number of times this source interrupt has been invoked with at least one hook function.
 * \param no_hook_cnt [OUT] Number of times this source interrupt has been invoked with no hook functions attached.
 * \param listeners   [OUT] List of module IDs currently listening to this source interrupt.
 *
 * \return : Return code. Since this is an application-internal API, a trace error
 * is thrown in case of errors.
 */
mesa_rc vtss_interrupt_source_info_get(meba_event_t source_id, u32 *hook_cnt, u32 *no_hook_cnt, vtss::Vector<vtss_module_id_t> &listeners);

/**
 * \brief Enable individual interrupt event sources.
 *
 * \param event [IN] The interrupt event to control
 *
 * \return Return code.
 **/
typedef mesa_rc (*vtss_interrupt_event_enable_t)(meba_event_t event);

/**
 * \brief Handle low-level chip interrupt, generating
 * generic, application-level interrupts.
 *
 * \param chip_irq        [IN] Chip interrupt which triggered
 * \param signal_notifier [IN] Function to deliver decoded interrupts to
 *
 * \return Return code.
 **/
typedef mesa_rc (*vtss_interrupt_irq_handler_t)(mesa_irq_t chip_irq,
                                                meba_event_signal_t signal_notifier);

/**
 * Register an interrupt handler
 *
 * \param module_id     [IN] Module id of handler
 * \param chip_irq      [IN] IRQ number registering
 * \param irq_handler   [IN] IRQ handler function pointer. Must return MESA_RC_OK iff irq handled.
 *
 * \return : Return code.
 */
mesa_rc vtss_interrupt_handler_add(vtss_module_id_t module_id,
                                   mesa_irq_t chip_irq,
                                   vtss_interrupt_irq_handler_t irq_handler);
/**
 * Top-level interrupt handler
 *
 * \param chip_irq        [IN] Chip IRQ seen
 * \param signal_notifier [IN] Callout for decoded interrupts
 *
 * \return : Return code.
 */
mesa_rc vtss_interrupt_handler(mesa_irq_t chip_irq,
                               meba_event_signal_t signal_notifier);

/**
 * Internal function for initializing the interrupt module.
 *
 * \return : Return code.
 */
mesa_rc interrupt_init(vtss_init_data_t *data);

#ifdef __cplusplus
}
#endif

// Allow interation on meba_event_t enum
VTSS_ENUM_INC(meba_event_t);

#endif /* _VTSS_INTERRUPT_API_H_ */

