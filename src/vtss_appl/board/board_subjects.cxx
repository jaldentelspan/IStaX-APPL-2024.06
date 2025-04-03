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

#include <vtss/basics/notifications.hxx>
#include <vtss/basics/array.hxx>
#include <vtss/basics/notifications/event-handler.hxx>
#include <vtss/basics/map.hxx>

#include "board_subjects.hxx"
#include "port_api.h"  // For port_phy_wait_until_ready
#include "interrupt.h" /* For VTSS_TRACE_MODULE_ID && TRACE_GRP_IRQ */

static int __irq_fd;
static const char *__uio;
static vtss::Map<std::string, mesa_irq_t> irqmap;

using namespace vtss::notifications;

namespace vtss {
namespace notifications {
SubjectRunner subject_irq_thread("IRQ Thread", VTSS_TRACE_MODULE_ID, true);
} // namespace notifications


vtss::Array<BoardIrq*, MESA_IRQ_MAX> irq_list;

mesa_rc meba_irq_handler_(mesa_irq_t chip_irq,
                          meba_event_signal_t signal_notifier)
{
    return meba_irq_handler(board_instance, chip_irq, signal_notifier);
}

// Class for handling an event on the irq_fd file descriptor
// The irq_fd is a file descriptor for UIO, which is the only FD, the kernel can
// signal events on.
// In this class, a special file is opened and read from to determine which
// interrupts are currently active.
// So the class listens on irq_fd, but reads from irqctl_rd FD.
// In order to disable or re-enable interrupts in the kernel, another FD
// (irqctl_wr) is needed that allows for writing to the special file.
// One could probably use the same FD for both reading and writing the special
// file, but the file offset would have to be adjusted for every write in order
// to get all lines out - hence we have both a read and a write FD.
struct IrqFdEventHandler : public EventHandler {
    IrqFdEventHandler(int irq_fd) : EventHandler(&subject_irq_thread), irq_event_fd(this) {
        T_IG(TRACE_GRP_IRQ, "irq_fd = %d", irq_fd);

        // After this assignment, execute() gets called on every event on irq_fd.
        irq_event_fd.assign(Fd(irq_fd));
        subject_irq_thread.event_fd_add(irq_event_fd, EventFd::READ);
    }

    char nm_irqctl[NAME_MAX + 64];
    u8 irq_faults[MESA_IRQ_MAX];
    FILE *irqctl_rd, *irqctl_wr;

    void initialize() {
        const int enable = 1;
        int i;

        T_IG(TRACE_GRP_IRQ, "Wait for PHY init");
        port_phy_wait_until_ready();

        // Reset point
        meba_reset(board_instance, MEBA_INTERRUPT_INITIALIZE);

        // reset counters for interrupt fails
        for (i=0;i<MESA_IRQ_MAX;i++) {
            irq_faults[i] = 0;
        }

        sprintf(nm_irqctl, "%s/device/irqctl", __uio);
        T_IG(TRACE_GRP_IRQ, "fopen(%s)", nm_irqctl);
        irqctl_rd = fopen(nm_irqctl,"r");
        if (!irqctl_rd) {
            T_EG(TRACE_GRP_IRQ, "Failed to open %s for reading", nm_irqctl);
        }

        irqctl_wr = fopen(nm_irqctl,"w");
        if (!irqctl_wr) {
            T_EG(TRACE_GRP_IRQ, "Failed to open %s for writing", nm_irqctl);
        }

        // Line buffered output.
        setlinebuf(irqctl_wr);

        // Detect which IRQ are handled by MEBA
        for (auto irq = (mesa_irq_t) 0; irq < MESA_IRQ_MAX; irq++) {
            if (meba_irq_requested(board_instance, irq) == MESA_RC_OK) {
                T_IG(TRACE_GRP_IRQ, "MEBA handles IRQ %d", irq);
                vtss_interrupt_handler_add(VTSS_MODULE_ID_MEBA, irq, meba_irq_handler_);
            }
        }

        if (write(irq_event_fd.raw(), &enable, sizeof(enable)) != sizeof(enable)) {
            T_EG(TRACE_GRP_IRQ, "Unable to enable IRQs: %s", strerror(errno));
        }
    }

    // Callback when something happens on the IRQ file descriptor
    void execute(EventFd *e) override {
        int               irq_seq_num;
        const int         enable = 1;

        if (!(e->events() & EventFd::READ)) {
            T_EG(TRACE_GRP_IRQ, "Unexpected event");
            return;
        }

        if (read(e->raw(), &irq_seq_num, sizeof(irq_seq_num)) != sizeof(irq_seq_num)) {
            T_EG(TRACE_GRP_IRQ, "Read IRQ error: %s", strerror(errno));
            return;
        } else {
            char line[128];

            // Enable IRQ's again
            if (write(e->raw(), &enable, sizeof(enable)) != sizeof(enable)) {
                T_EG(TRACE_GRP_IRQ, "write() failed. Unable to enable IRQs");
            }

            T_NG(TRACE_GRP_IRQ, "Got event on %d. Reading %s, irq_seq_num = %d", e->raw(), nm_irqctl, irq_seq_num);

            rewind(irqctl_rd);
            rewind(irqctl_wr);

            // This function assumes that all handlers have been installed prior
            // to starting, because if an interrupt handler is not installed by
            // now and an interrupt occurs on an irq_id, that irq_id will be
            // disabled and can't become re-enabled. See also comment in
            // interrupt.cxx#vtss_interrupt_handler_add().
            while (fgets(line, sizeof(line), irqctl_rd)) {
                int  irq_id, ct;
                char irq_name[32];

                line[strlen(line) - 1] = '\0'; // Strip trailing \n
                T_NG(TRACE_GRP_IRQ, ">>> fgets returned %s * ", line);
                if ((ct = sscanf(line, "%d|%31s", &irq_id, irq_name)) == 2) {
                    bool disable = false;
                    auto iter = irqmap.find(irq_name);

                    if (iter != irqmap.end()) {
                        mesa_irq_t irq = iter->second;
                        BoardIrq *e = irq_list[irq];
                        if (e) {
                            e->tally();
                            T_IG(TRACE_GRP_IRQ, "Kernel irq_id = %d => MESA_IRQ = %d (%s), cnt %d", irq_id, irq, irq_name, e->cnt());
                            meba_irq_enable(board_instance, irq, enable);

                            if (vtss_interrupt_handler(irq, interrupt_source_signal) != VTSS_RC_OK) {
                                /* APPL-5354:
                                * Added irq_faults due to situtations where ptp_event_poll was called without an event on Caracal
                                * which would end up disabling the MESA_IRQ_PTP_SYNC IRQ hence also PTP communication.
                                * This change will still catch potential interrupt storms as irq_faults is limited to 5 faults in a row.
                                */
                                irq_faults[irq]++;
                                T_IG(TRACE_GRP_IRQ, "Detected %u irq fault(s) on handler for kernel_irq_id = %d = MESA_IRQ = %d (%s).", irq_faults[irq], irq_id, irq, irq_name);
                                if (irq_faults[irq] > ALLOWED_IRQ_FAULTS) {
                                    // Disable IRQ
                                    T_IG(TRACE_GRP_IRQ, "No handler for kernel_irq_id = %d = MESA_IRQ = %d (%s). Disabling.", irq_id, irq, irq_name);
                                    disable = true;
                                }
                            } else {
                                irq_faults[irq] = 0;
                            }
                        } else {
                            // Disable IRQ
                            T_EG(TRACE_GRP_IRQ, "Got kernel irq_id = %d = MESA_IRQ = %d (%s) not supported by board. Disabling.", irq_id, irq, irq_name);
                        }
                    } else {
                        // Disable IRQ
                        T_IG(TRACE_GRP_IRQ, "Got unknown kernel irq_id = %d (%s). Disabling.", irq_id, irq_name);
                        disable = true;
                    }

                    // Either re-enable or disable the IRQ.
                    T_IG(TRACE_GRP_IRQ, " %sABLE IRQ (Kernel irq %d) (seq %d)", disable ? "DIS" : "RE-EN", irq_id, irq_seq_num);
                    fprintf(irqctl_wr, "%d\n", disable ? -irq_id : irq_id);
                } else {
                    T_DG(TRACE_GRP_IRQ, "Skip line: %s", line);
                }
            }
            T_NG(TRACE_GRP_IRQ, "=================== Done handling event (seq_num %d) ===========", irq_seq_num);
        }

        subject_irq_thread.event_fd_add(irq_event_fd, EventFd::READ);
    }

private:
    EventFd irq_event_fd;
};

static void subject_irq_thread_run(vtss_addrword_t data)
{
    static struct IrqFdEventHandler irq_fd_event_handler(__irq_fd);
    // Gotta wait until the PHYs are initialized until
    // we start generating interrupts towards the application.
    //VTSS_OS_MSLEEP(10000);
    T_D("Initialize MEBA interrupts");
    irq_fd_event_handler.initialize();
    T_D("IRQ thread starting");
    subject_irq_thread.run();
}

void board_subjects_start(int irq_fd, const char *uio)
{
    vtss_handle_t thread_handle;
    __irq_fd = irq_fd;
    __uio = uio;

    // Establish string name to MESA IRQ source mapping
    irqmap["ptp_rdy"] = MESA_IRQ_PTP_RDY;
    irqmap["ptp_sync"] = MESA_IRQ_PTP_SYNC;
    irqmap["ext_src0"] = MESA_IRQ_EXT0;
    irqmap["ext_src1"] = MESA_IRQ_EXT1;
    irqmap["oam_vop"] = MESA_IRQ_OAM;
    irqmap["sgpio"] = MESA_IRQ_SGPIO;
    irqmap["sgpio0"] = MESA_IRQ_SGPIO;
    irqmap["sgpio2"] = MESA_IRQ_SGPIO2;
    irqmap["dpll"] = MESA_IRQ_DPLL;
    irqmap["gpio"] = MESA_IRQ_GPIO;
    irqmap["dev_all"] = MESA_IRQ_DEV_ALL;
    irqmap["cu_phy0"] = MESA_IRQ_CU_PHY_0;
    irqmap["cu_phy1"] = MESA_IRQ_CU_PHY_1;
    irqmap["pushbutton"] = MESA_IRQ_PUSH_BUTTON;
    irqmap["sd10g_kr0"] = MESA_IRQ_KR_SD10G_0;
    irqmap["sd10g_kr1"] = MESA_IRQ_KR_SD10G_1;
    irqmap["sd10g_kr2"] = MESA_IRQ_KR_SD10G_2;
    irqmap["sd10g_kr3"] = MESA_IRQ_KR_SD10G_3;
    irqmap["sd10g_kr4"] = MESA_IRQ_KR_SD10G_4;
    irqmap["sd10g_kr5"] = MESA_IRQ_KR_SD10G_5;
    irqmap["sd10g_kr6"] = MESA_IRQ_KR_SD10G_6;
    irqmap["sd10g_kr7"] = MESA_IRQ_KR_SD10G_7;
    irqmap["sd10g_kr8"] = MESA_IRQ_KR_SD10G_8;
    irqmap["sd10g_kr9"] = MESA_IRQ_KR_SD10G_9;
    irqmap["sd10g_kr10"] = MESA_IRQ_KR_SD10G_10;
    irqmap["sd10g_kr11"] = MESA_IRQ_KR_SD10G_11;
    irqmap["sd10g_kr12"] = MESA_IRQ_KR_SD10G_12;
    irqmap["sd10g_kr13"] = MESA_IRQ_KR_SD10G_13;
    irqmap["sd10g_kr14"] = MESA_IRQ_KR_SD10G_14;
    irqmap["sd10g_kr15"] = MESA_IRQ_KR_SD10G_15;
    irqmap["sd10g_kr16"] = MESA_IRQ_KR_SD10G_16;
    irqmap["sd10g_kr17"] = MESA_IRQ_KR_SD10G_17;
    irqmap["sd10g_kr18"] = MESA_IRQ_KR_SD10G_18;
    irqmap["sd10g_kr19"] = MESA_IRQ_KR_SD10G_19;

    vtss_thread_create(VTSS_THREAD_PRIO_HIGHER,
                       subject_irq_thread_run,
                       NULL,
                       const_cast<char *>(subject_irq_thread.name),
                       nullptr,
                       0,
                       &thread_handle,
                       NULL);
}

} // namespace vtss
