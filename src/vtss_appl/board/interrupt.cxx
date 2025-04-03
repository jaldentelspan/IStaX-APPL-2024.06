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

#include "critd_api.h"
#include "interrupt.h"
#include "interrupt_api.h"
#include "port_api.h"  // For port_phy_wait_until_ready
#include "board_if.h"
#include "board_subjects.hxx"
#include "vtss_api_if_api.h"       /* For vtss_api_if_chip_count() */
#include <vtss/basics/array.hxx>
#include <vtss/basics/vector.hxx>
#include <vtss/basics/list.hxx>

/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

/* Structure for global variables */
static critd_t intr_crit;

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_INTERRUPT

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "interrupt", "Interrupts"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    },
    [TRACE_GRP_IRQ] = {
        "IRQ",
        "IRQ",
        VTSS_TRACE_LVL_ERROR
    },
    [TRACE_GRP_CHIP] = {
        "chip",
        "Chip",
        VTSS_TRACE_LVL_ERROR
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define CRIT_ENTER() critd_enter(&intr_crit, __FILE__, __LINE__)
#define CRIT_EXIT()  critd_exit( &intr_crit, __FILE__, __LINE__)

#define IRQ_INIT(_source_) [MESA_IRQ_ ## _source_] = #_source_
static vtss::Array<const char *, MESA_IRQ_MAX> irq_name = {{
        IRQ_INIT(XTR),
        IRQ_INIT(FDMA_XTR),
        IRQ_INIT(SOFTWARE),
        IRQ_INIT(PTP_RDY),
        IRQ_INIT(PTP_SYNC),
        IRQ_INIT(EXT0),
        IRQ_INIT(EXT1),
        IRQ_INIT(OAM),
        IRQ_INIT(SGPIO),
        IRQ_INIT(SGPIO2),
        IRQ_INIT(DPLL),
        IRQ_INIT(GPIO),
        IRQ_INIT(PUSH_BUTTON),
        IRQ_INIT(DEV_ALL),
        IRQ_INIT(CU_PHY_0),
        IRQ_INIT(CU_PHY_1),
        IRQ_INIT(KR_SD10G_0),
        IRQ_INIT(KR_SD10G_1),
        IRQ_INIT(KR_SD10G_2),
        IRQ_INIT(KR_SD10G_3),
        IRQ_INIT(KR_SD10G_4),
        IRQ_INIT(KR_SD10G_5),
        IRQ_INIT(KR_SD10G_6),
        IRQ_INIT(KR_SD10G_7),
        IRQ_INIT(KR_SD10G_8),
        IRQ_INIT(KR_SD10G_9),
        IRQ_INIT(KR_SD10G_10),
        IRQ_INIT(KR_SD10G_11),
        IRQ_INIT(KR_SD10G_12),
        IRQ_INIT(KR_SD10G_13),
        IRQ_INIT(KR_SD10G_14),
        IRQ_INIT(KR_SD10G_15),
        IRQ_INIT(KR_SD10G_16),
        IRQ_INIT(KR_SD10G_17),
        IRQ_INIT(KR_SD10G_18),
        IRQ_INIT(KR_SD10G_19),
    }
};

struct IntrHook {
    vtss_interrupt_priority_t      priority;
    vtss_interrupt_function_hook_t function_hook;
    vtss_module_id_t               module_id;
};

// These handler *MESA*, low-level interrupts
struct IntrHandler {
    vtss_interrupt_irq_handler_t irq_handler;
    vtss_module_id_t             module_id;
};
static vtss::Array<vtss::List <IntrHandler>, MESA_IRQ_MAX> irq_handlers;

struct IntrSource {
    constexpr IntrSource(const char *n) :
        name(n) {}

    const char             *name;           // What's the name of this source interrupt?
    vtss::Vector<IntrHook> hooks;           // Who's listening?
    u32                    hook_cnt = 0;    // How many times has this been invoked with at least one hook function?
    u32                    no_hook_cnt = 0; // How many times has this been invoked with no hook functions?
};

// By using an explicitly sized array, we make sure that new interrupt
// sources also get into this array, because MEBA_EVENT_LAST will increment
// and without the new entry, a compilation error will occur because not all
// elements are specified.
// By using designated initialization ([12] = {"bla-bla"}), we ensure
// there is a one-to-one correspondence between MEBA_EVENT_xxx and
// array index.
#define INTR_SOURCE_INIT(_source_) [MEBA_EVENT_ ## _source_] = {#_source_}
static vtss::Array<IntrSource, MEBA_EVENT_LAST> INTR_sources = {{
        INTR_SOURCE_INIT(LOS),
        INTR_SOURCE_INIT(FLNK),
        INTR_SOURCE_INIT(AMS),
        INTR_SOURCE_INIT(VOE),
        INTR_SOURCE_INIT(SYNC),
        INTR_SOURCE_INIT(EXT_SYNC),
        INTR_SOURCE_INIT(EXT_1_SYNC),
        INTR_SOURCE_INIT(CLK_ADJ),
        INTR_SOURCE_INIT(CLK_TSTAMP),
        INTR_SOURCE_INIT(PTP_PIN_0),
        INTR_SOURCE_INIT(PTP_PIN_1),
        INTR_SOURCE_INIT(PTP_PIN_2),
        INTR_SOURCE_INIT(PTP_PIN_3),
        INTR_SOURCE_INIT(PTP_PIN_4),
        INTR_SOURCE_INIT(PTP_PIN_5),
        INTR_SOURCE_INIT(INGR_ENGINE_ERR),
        INTR_SOURCE_INIT(INGR_RW_PREAM_ERR),
        INTR_SOURCE_INIT(INGR_RW_FCS_ERR),
        INTR_SOURCE_INIT(EGR_ENGINE_ERR),
        INTR_SOURCE_INIT(EGR_RW_FCS_ERR),
        INTR_SOURCE_INIT(EGR_TIMESTAMP_CAPTURED),
        INTR_SOURCE_INIT(EGR_FIFO_OVERFLOW),
        INTR_SOURCE_INIT(PUSH_BUTTON),
        INTR_SOURCE_INIT(MOD_DET),
        INTR_SOURCE_INIT(KR),
    }
};

static u32 interrupt_source_signal_call_cnt;

u32 interrupt_source_signal_call_cnt_get(bool clear)
{
    u32 result = interrupt_source_signal_call_cnt;

    if (clear) {
        interrupt_source_signal_call_cnt = 0;
    }

    return result;
}

void interrupt_source_signal(meba_event_t source_id, uint32_t instance_no)
{
    IntrSource *s;

    interrupt_source_signal_call_cnt++;

    if (source_id < 0 || source_id >= MEBA_EVENT_LAST) {
        T_E("Invalid source ID (%d)", source_id);
        return;
    }

    s = &INTR_sources[source_id];

    T_D(" INTERRUPT_SOURCE_SIGNAL: source_id = %d (%s), instance_no = %u (%d hooks exists)", source_id, s->name, instance_no, s->hooks.size());

    // Make a local copy of all the function hooks listening
    // on this source, so that we can release the crit before
    // calling back.
    CRIT_ENTER();

    vtss::Vector<IntrHook> local_copy(s->hooks);

    if (s->hooks.size()) {
        // At least one hook function.
        s->hook_cnt++;

        // Remove them all now.
        s->hooks.clear();

    } else {
        // No one is listening to this source.
        T_D("No listeners on %s", s->name);
        s->no_hook_cnt++;
    }

    CRIT_EXIT();

    for (auto itr = local_copy.cbegin(); itr != local_copy.cend(); ++itr) {
        T_D("Invoking hook for module %d (%s)", itr->module_id, vtss_module_names[itr->module_id]);
        itr->function_hook(source_id, instance_no);
    }
}

mesa_rc vtss_interrupt_source_hook_set(vtss_module_id_t               module_id,
                                       vtss_interrupt_function_hook_t function_hook,
                                       meba_event_t                   event_id,
                                       vtss_interrupt_priority_t      priority)
{
    IntrSource  *s;
    const char  *module_name;
    static bool phy_ready;
    mesa_rc     rc;

    // Check event_id and module_id first, because they are used further down
    // to show other error messages.
    if (event_id < 0 || event_id >= MEBA_EVENT_LAST) {
        T_E("Invalid source ID (%d)", event_id);
        return VTSS_RC_ERROR;
    }

    s = &INTR_sources[event_id];

    if (module_id >= VTSS_MODULE_ID_NONE) {
        T_E("Invalid module ID (%d) when attempting to hook source ID = %s", module_id, s->name);
        return VTSS_RC_ERROR;
    }

    module_name = vtss_module_names[module_id];

    if (function_hook == NULL) {
        T_E("%s: Invalid function hook for source ID = %s", module_name, s->name);
        return VTSS_RC_ERROR;
    }

    if (priority < 0 || priority >= INTERRUPT_PRIORITY_LAST) {
        T_E("%s: Invalid priority (%d) for source ID = %s", module_name, priority, s->name);
        return VTSS_RC_ERROR;
    }

    // We can't hook interrupts in the PHYs until they are up
    if (!phy_ready) {
        // Only do this once, which is during boot.
        port_phy_wait_until_ready();
        phy_ready = true;
    }

    CRIT_ENTER();

    // Check if #function_hook is already in the list of function hooks
    {
        auto itr = vtss::find_if(s->hooks.cbegin(), s->hooks.cend(), [&] (const IntrHook & h) {
            return h.function_hook == function_hook;
        });

        if (itr != s->hooks.cend()) {
            // Cannot currently promote this to a T_E(), because a lot of modules
            // currently re-insert there function hook.
            T_N("%s: Function hook already set for source = %s * ", module_name, s->name);
            rc = VTSS_RC_ERROR;
            goto do_exit;
        }
    }

    // Insert #function_hook in prioritized list
    {
        auto itr = vtss::find_if(s->hooks.cbegin(), s->hooks.cend(), [&] (const IntrHook & h) {
            // We want higher values of #priority to come first in the vector.
            return h.priority <= priority;
        });

        if (!s->hooks.emplace(itr, IntrHook {priority, function_hook, module_id})) {
            T_E("%s: Unable to store function hook for %s in hook vector", module_name, s->name);
            rc = VTSS_RC_ERROR;
            goto do_exit;
        } // "astyle" insists on a line-feed here.
        else {
            T_D("%s: Function hook inserted for %s", module_name, s->name);
        }
    }

    rc = MESA_RC_OK;

do_exit:
    CRIT_EXIT();

    if (rc == MESA_RC_OK) {
        T_D(" *** (re)set hook for event %d to module %d (%s)", event_id, module_id, s->name);
        // Enable source interrupt (not IRQ, which is supposedly already enabled).
        if ((rc = meba_event_enable(board_instance, event_id, true)) == MESA_RC_OK) {
            T_D("    set hook for %s (module %d) DONE", s->name, module_id);
        } else {
            T_W("No enable support for event %s", s->name);
        }
    }

    return rc;
}

mesa_rc vtss_interrupt_source_hook_clear(vtss_interrupt_function_hook_t function_hook,
                                         meba_event_t        source_id)
{
    IntrSource *s;
    mesa_rc    rc;

    // Check source_id first, because it is used further down to show other error messages.
    if (source_id < 0 || source_id >= MEBA_EVENT_LAST) {
        T_E("Invalid source ID (%d)", source_id);
        return VTSS_RC_ERROR;
    }

    s = &INTR_sources[source_id];

    if (function_hook == NULL) {
        T_E("Invalid function hook for source ID = %s", s->name);
        return VTSS_RC_ERROR;
    }

    CRIT_ENTER();

    // Find #function_hook in the list of function hooks
    auto itr = vtss::find_if(s->hooks.cbegin(), s->hooks.cend(), [&] (const IntrHook & h) {
        return h.function_hook == function_hook;
    });

    if (itr != s->hooks.cend()) {
        T_D("%s: Removing function hook for %s", vtss_module_names[itr->module_id], s->name);
        s->hooks.erase(itr);
        rc = MESA_RC_OK;
    } else {
        T_W("Function hook not found for %s", s->name);
        rc = VTSS_RC_ERROR;
    }

    CRIT_EXIT();
    return rc;
}

const char *vtss_interrupt_source_text(meba_event_t source_id)
{
    if (source_id < 0 || source_id >= MEBA_EVENT_LAST) {
        T_E("Invalid source ID (%d)", source_id);
        return "";
    }

    return INTR_sources[source_id].name;
}

mesa_rc vtss_interrupt_source_info_get(meba_event_t source_id, u32 *hook_cnt, u32 *no_hook_cnt, vtss::Vector<vtss_module_id_t> &listeners)
{
    IntrSource *s;

    if (source_id < 0 || source_id >= MEBA_EVENT_LAST) {
        T_E("Invalid source ID (%d)", source_id);
        return VTSS_RC_ERROR;
    }

    if (hook_cnt == NULL || no_hook_cnt == NULL) {
        T_E("OUT-params must be non-NULL");
        return VTSS_RC_ERROR;
    }

    s = &INTR_sources[source_id];

    CRIT_ENTER();
    *hook_cnt     = s->hook_cnt;
    *no_hook_cnt  = s->no_hook_cnt;

    for (auto itr = s->hooks.cbegin(); itr != s->hooks.cend(); ++itr) {
        listeners.push_back(itr->module_id);
    }

    CRIT_EXIT();
    return MESA_RC_OK;
}

mesa_rc vtss_interrupt_handler_add(vtss_module_id_t module_id,
                                   mesa_irq_t irq,
                                   vtss_interrupt_irq_handler_t irq_handler)
{
    mesa_rc rc = MESA_RC_ERROR;
    T_D("Add interrupt handler for owner %d (%d) ", module_id, irq);
    if (irq < irq_handlers.size()) {
        if (fast_cap(MESA_CAP_MISC_IRQ_CONTROL)) {
            mesa_bool_t external = (fast_cap(MESA_CAP_MISC_CPU_TYPE) == MESA_CPU_TYPE_EXTERNAL);
            mesa_irq_conf_t conf;
            if (mesa_irq_conf_get(NULL, irq, &conf) == MESA_RC_OK) {
                conf.external = external;
                conf.destination = 0; // CPU0 = INT0_IRQ (BSP), CPU1 = INT1_IRQ (UIO)
                mesa_irq_conf_set(NULL, irq, &conf);
            } else {
                T_WG(TRACE_GRP_IRQ, "No IRQ #%d config?", irq);
            }
        }
        meba_irq_enable(board_instance, irq, true);
        // Track the list of IRQ for statistics
        if (vtss::irq_list[irq] == nullptr) {
            vtss::irq_list[irq] = new vtss::BoardIrq(irq, irq_name[irq]);
        }
        CRIT_ENTER();
        irq_handlers[irq].push_back(IntrHandler {irq_handler, module_id});
        CRIT_EXIT();
        rc = MESA_RC_OK;
    }

    // In principle, the kernel must also be told that this interrupt should be
    // enabled, because in board_subjects.cxx, execute() disables interrupts
    // from sources that don't have an interrupt handler installed. However,
    // it's not possible to find the kernel's irq_id (which is zero-based, and
    // may differ from platform to platform) based on a MESA_IRQ (e.g.
    // MESA_IRQ_PTP_SYNC). The kernel's uio_fireant_irqmux.c (used by all
    // platforms) could be modified in its irqctl_show() function to always
    // print out all interrupt names and irq_ids to allow the application to
    // find a mapping.
    // Also, it is currently not possible to enable a disabled interrupt in the
    // kernel. It could be made possible by modifying irqctl_store() in
    // the kernel's uio_fireant_irq_mux() to always do an enable_irq() whether
    // the interrupt is currently active or not.
    return rc;
}

mesa_rc vtss_interrupt_handler(mesa_irq_t irq,
                               meba_event_signal_t signal_notifier)
{
    int handled = 0;
    if (irq < irq_handlers.size()) {
        auto handlers = irq_handlers[irq];
        for (IntrHandler const &handler : handlers) {
            if (handler.irq_handler(irq, signal_notifier) == MESA_RC_OK) {
                T_D("Owner %d (%s) handled irq %d", handler.module_id,  vtss_module_names[handler.module_id], irq);
                handled++;
            }
        }
    }
    return handled == 0 ? MESA_RC_NOT_IMPLEMENTED : MESA_RC_OK;
}

extern "C" int interrupt_icli_cmd_register();

// Initialize module
mesa_rc interrupt_init(vtss_init_data_t *data)
{
    if (data->cmd == INIT_CMD_INIT) {
        critd_init(&intr_crit, "Interrupt", VTSS_MODULE_ID_INTERRUPT, CRITD_TYPE_MUTEX);
        interrupt_icli_cmd_register();
    }

    return VTSS_RC_OK;
}

