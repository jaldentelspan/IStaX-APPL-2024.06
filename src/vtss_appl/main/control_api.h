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

#ifndef _CONTROL_API_H_
#define _CONTROL_API_H_

#include "vtss_os_wrapper.h"
#include "main_types.h" /* Types */
#include "microchip/ethernet/switch/api.h" /* For mesa_restart_t */

#ifdef __cplusplus
extern "C" {
#endif

/* Control messages IDs */
typedef enum {
    CONTROL_MSG_ID_SYSTEM_REBOOT, /* Reboot system */
    CONTROL_MSG_ID_CONFIG_RESET,  /* Reset config (restore default) */
} control_msg_id_t;

typedef struct main_msg {
    control_msg_id_t  msg_id;
    vtss_flag_value_t flags;
} control_msg_t;

/*
 * Reset the configuration syncronously in the context of the current thread.
 */
void control_config_reset(vtss_usid_t usid, vtss_flag_value_t flags);

/*
 * Reset the configuration asyncronously by letting the main thread do the job.
 */
void control_config_reset_async(vtss_usid_t usid, vtss_flag_value_t flags);


mesa_rc control_system_restart_status_get(const mesa_inst_t inst,
                                          mesa_restart_status_t *const status);
// Convert the restart type to a string.
const char *control_system_restart_to_str(mesa_restart_t restart);

/* System reset */
void control_system_reset_sync(mesa_restart_t restart)  __attribute__ ((noreturn));
void control_system_reset_no_cb(mesa_restart_t restart)  __attribute__ ((noreturn));
/* Retrun 0: reset success, -1: reset fail (System is updating by another process) */
int control_system_reset(BOOL local, vtss_usid_t usid, mesa_restart_t restart, uint32_t wait_before_callback_msec = 0);
void control_system_flash_lock(void);
void control_system_flash_unlock(void);

/* Register for reset early warning (1 second) */
typedef void (*control_system_reset_callback_t)(mesa_restart_t restart);


typedef enum {
    CONTROL_SYSTEM_RESET_PRIORITY_LOW,
    CONTROL_SYSTEM_RESET_PRIORITY_NORMAL,
} control_system_reset_priority_t;

/*
 * NB: The callback should be NON-BLOCKING, and return asap.
 * Execution started on other threads is given 1 second grace to complete.
 */
void control_system_reset_register(control_system_reset_callback_t cb, vtss_module_id_t module_id, control_system_reset_priority_t prio = CONTROL_SYSTEM_RESET_PRIORITY_NORMAL);

void dump_exception_data(int (*pr)(const char *fmt, ...), vtss_code_t exception_number, const HAL_SavedRegisters *regs, BOOL dump_threads);

// Overloaded functions that first acquire a flash semaphore.
// These functions have the same signatures, except for the initial
// "_printf *pf", which is passed to flash_init() in case the flash
// is not already initialized or the function has changed since
// the last call to control_flash_erase() or control_flash_program().
int control_flash_erase(vtss_flashaddr_t base, size_t len);
int control_flash_program(vtss_flashaddr_t flash_base, const void *ram_base, size_t len);
int control_flash_read(vtss_flashaddr_t flash_base, void *dest, size_t len);
void control_flash_get_info(vtss_flashaddr_t *start, size_t *size);

// Retrieve current system load
vtss_bool_t control_sys_get_cpuload(u32 *average_point1s, u32 *average_1s, u32 *average_10s);

// Debug stuff
void control_dbg_latest_init_modules_get(vtss_init_data_t *data, const char **init_module_func_name);

#ifdef __cplusplus
}
#endif

#endif // _CONTROL_API_H_

