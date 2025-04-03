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

#ifndef __ICFG_H__
#define __ICFG_H__

#include "icfg_api.h"
#include <sys/stat.h>
#include "os_file_api.h"

// Maximum number of writable files in flash: fs (fs sync-to-flash implementation
// limitation). This allows for 'startup-config' + n other files.
#define ICFG_MAX_WRITABLE_FILES_IN_FLASH_CNT 32

#if (ICFG_MAX_WRITABLE_FILES_IN_FLASH_CNT > OS_FILE_FILES_MAX)
#error Adjust OS_FILE_FILES_MAX for file persistence of desired capacity
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ICFG_COMMIT_IDLE,
    ICFG_COMMIT_RUNNING,
    ICFG_COMMIT_DONE,
    ICFG_COMMIT_SYNTAX_ERROR,
    ICFG_COMMIT_ERROR
} icfg_commit_state_t;

/** \brief Return number of R/O and R/W files in the flash: file system.
    \return FALSE == error, don't trust results; TRUE = all is well.
*/
BOOL icfg_flash_file_count_get(u32 *ro_count, u32 *rw_count);

/** \section Configuration File Load.
 *
 * \details When a set of lines have been assembled, usually through the loading
 * of a file from the local file system, from a TFTP server, or via the web UI,
 * they must be submitted to ICLI for processing and execution.
 */

/** \brief  Save a buffer to a file name.
 *  \return NULL if save was OK, pointer to constant string error message
 *          otherwise
 */
const char *icfg_file_write(const char *filename, vtss_icfg_query_result_t *res, bool is_flash_file = true);

/** \brief  Load file from flash into a buffer.
 *  \return NULL if load was OK, pointer to constant string error message
 *          otherwise.
 */
const char *icfg_file_read(const char *filename, vtss_icfg_query_result_t *res, bool is_flash_file = true);

/** \brief  Delete file.
 *  \return NULL if delete was OK, pointer to constant string error message
 *          otherwise.
 */
const char *icfg_file_delete(const char *filename, bool is_flash_file = true);

/** \brief Stat a file that's under ICFG control. Semantically equivalent to
 *         the stdlib stat() function except for the additional compressed_size
 *         parameter, but hides some ugliness about file permissions and size.
 */
int icfg_file_stat(const char *path, struct stat *buf, off_t *compressed_size, bool is_flash_file = true);

/** \brief  Get the free flash size for files.
 *  \param flash_free       The free flash size availabe for writing config files.
 *  \param total_flash_free The total free flash size, including flash memory
 *                          reserved for firmware upgrades.
 *  \return OK
 */
mesa_rc icfg_flash_free_get(const struct statvfs &fs_buf, u32 &flash_free, u32 &total_flash_free);


/** \brief Return status of most recently submitted commit.
 *
 *  \param running   [OUT] Current commit state
 *  \param error_cnt [OUT] Error count. Not valid while commit is running.
 */
void icfg_commit_status_get(icfg_commit_state_t *state, u32 *error_cnt);

/** \brief Trigger commit thread.
 *  \return NULL if trigger was OK, pointer to constant string error message
 *          otherwise
 */
const char *icfg_commit_trigger(const char *filename, vtss_icfg_query_result_t *buf);

/** \brief Wait for load to complete. Returns immediately if no load is in
 *         progress; otherwise blocks.
 */
void icfg_commit_complete_wait(void);

/** \brief Return pointer to output buffer and its length. The buffer is
 *         updated when icfg_commit_to_icli() is called with
 *         ICLI_SESSION_ID_NONE + use_output_buffer.
 */
const char *icfg_commit_output_buffer_get(u32 *length);

/** \brief Append string to output buffer. */
void icfg_commit_output_buffer_append(const char *str);

/** \section Load/Save mutex.
 *
 * \details This mutex is used for protecting whole-file load/save operations:
 *          Max one can be in progress at any time; overlapping requests must
 *          be denied with an error message
 */

/** \brief Try to lock the mutex.
 *
 * \return TRUE == obtained lock; FALSE == already locked, can't load/save now
 */
BOOL icfg_try_lock_io_mutex(void);

/** \brief Unlock the mutex.
 */
void icfg_unlock_io_mutex(void);



#ifdef __cplusplus
}
#endif
#endif /* ICFG_H */
