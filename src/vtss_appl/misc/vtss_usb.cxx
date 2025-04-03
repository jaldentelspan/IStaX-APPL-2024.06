/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include <vtss_trace_lvl_api.h>
#include <vtss_trace_api.h>
#include "misc.h"
#include "critd_api.h"
#include "vtss_os_wrapper.h"
#include "vtss_usb.h"
#include <sys/stat.h>
#include <sys/mount.h>

#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#else
/* Define dummy syslog macros */
#define S_I(fmt, ...)
#define S_W(fmt, ...)
#define S_E(fmt, ...)
#endif

/* Thread variables */
static vtss_handle_t usb_thread_handle;
static vtss_thread_t usb_thread_block;
static bool device_present;

static void usb_thread(vtss_addrword_t data)
{
    struct stat info;

    T_DG(TRACE_GRP_USB, "Entering usb_device_thread");
    for (;;) {
        VTSS_OS_MSLEEP(1000);
        if (stat("/dev/sda1", &info) != -1) {
            if (device_present) {
                // no action
            } else {
                T_DG(TRACE_GRP_USB, "device /dev/sda1 detected - try to mount it");
                if (mount("/dev/sda1", USB_DEVICE_DIR, "msdos", 0, "") == 0) {
                    T_IG(TRACE_GRP_USB, "USB device /dev/sda1 mounted on %s", USB_DEVICE_DIR);
                    S_I("USB device /dev/sda1 mounted to %s", USB_DEVICE_DIR);
                } else {
                    T_WG(TRACE_GRP_USB, "Error %s %d during mount of /dev/sda1", strerror(errno), errno);
                }
                device_present = true;
            }
        } else {
            if (device_present) {
                T_DG(TRACE_GRP_USB, "device /dev/sda1 removed - try to umount it");
                if (umount(USB_DEVICE_DIR) == 0) {
                    T_IG(TRACE_GRP_USB, "USB device /dev/sda1 removed and %s umounted", USB_DEVICE_DIR);
                    S_I("USB device /dev/sda1 removed and %s umounted", USB_DEVICE_DIR);
                } else {
                    T_WG(TRACE_GRP_USB, "Error %s %d during umount of %s", strerror(errno), errno, USB_DEVICE_DIR);
                }
                device_present = false;
            } else {
                // no action
            }
        }
    }
}

bool usb_is_device_present(void)
{
    return (device_present);
}

mesa_rc usb_init(void)
{
    device_present = false;
    // start thread and wait for register callback
    vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                       usb_thread,
                       0,
                       "USB",
                       nullptr,
                       0,
                       &usb_thread_handle,
                       &usb_thread_block);

    return VTSS_RC_OK;
    
}

