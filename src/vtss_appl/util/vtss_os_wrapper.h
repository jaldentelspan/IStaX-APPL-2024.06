/*

 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#ifndef __VTSS_OS_WRAPPER_H__
#define __VTSS_OS_WRAPPER_H__

#include <time.h>
#include "main_types.h"

// For OSs that have both a real-time and a normal scheduling mode
// (e.g. Linux), we use this enum to both select one or the other mode
// and to control the "priority" of the given mode.
// For OSs that only have one scheduling mode, there is no difference
// between selecting e.g. VTSS_THREAD_PRIO_NORMAL and VTSS_THREAD_PRIO_NORMAL_RT
//
// On Linux, selecting an VTSS_THREAD_PRIO_xxx_RT value causes the created
// thread to run with the round-robin real-time scheduling policy (SCHED_RR)
// and with a given mapping of the "xxx"-part to a real-time priority.
// The non-real-time variants will cause the thread to be created with
// a normal scheduling policy (SCHED_NORMAL), and the "xxx"-part will map
// to a nice-value (which controls the length of a given thread's timeslice
// rather than its priority).
//
// On Linux, real-time threads will always run before non-real-time ones.
typedef enum {
    // Non-real-time "priority" controlled through niceness.
    VTSS_THREAD_PRIO_BELOW_NORMAL,
    VTSS_THREAD_PRIO_DEFAULT,
    VTSS_THREAD_PRIO_ABOVE_NORMAL,
    VTSS_THREAD_PRIO_HIGH,
    VTSS_THREAD_PRIO_HIGHER,
    VTSS_THREAD_PRIO_HIGHEST,

    // Real-time priority
    VTSS_THREAD_PRIO_BELOW_NORMAL_RT,
    VTSS_THREAD_PRIO_DEFAULT_RT,
    VTSS_THREAD_PRIO_ABOVE_NORMAL_RT,
    VTSS_THREAD_PRIO_HIGH_RT,
    VTSS_THREAD_PRIO_HIGHER_RT,
    VTSS_THREAD_PRIO_HIGHEST_RT,

    // This cannot be used, but may be returned if the underlying OS-native
    // priority cannot be converted to one of the above
    VTSS_THREAD_PRIO_NA
} vtss_thread_prio_t;

typedef u32 vtss_flag_value_t;

#include "vtss_os_wrapper_linux.h"

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/* Thread operations */
typedef void vtss_thread_entry_f(vtss_addrword_t);
void vtss_thread_create(vtss_thread_prio_t  priority,    // Thread priority (VTSS_THREAD_PRIO_xxx; e.g. VTSS_THREAD_PRIO_DEFAULT)
                        vtss_thread_entry_f *entry,      // Entry point function
                        vtss_addrword_t     entry_data,  // Entry data
                        const char          *name,       // Thread name
                        void                *stack_base, // Stack base (unused, set to nullptr)
                        u32                 stack_size,  // Stack size (unused, set to 0)
                        vtss_handle_t       *handle,     // Returned thread handle
                        vtss_thread_t       *thread);    // Put thread here (not used on Linux)
vtss_bool_t        vtss_thread_delete(vtss_handle_t thread_handle); // false if NOT deleted. Use with care
void               vtss_thread_yield(void);
vtss_handle_t      vtss_thread_self(void);
void               vtss_thread_prio_set(vtss_handle_t thread_handle, vtss_thread_prio_t priority);
const char        *vtss_thread_prio_to_txt(vtss_thread_prio_t prio);
vtss_handle_t      vtss_thread_get_next(vtss_handle_t thread_handle);
int                vtss_thread_id_get(void); // Of currently running thread
vtss_handle_t      vtss_thread_handle_from_id(int id); // returns 0 if not found.
int                vtss_thread_id_from_thread_stack_range_get(void *lo, void *hi); // returns 0 if not found
vtss_addrword_t   *vtss_thread_get_data_ptr(u32 index);
vtss_thread_key_t  vtss_thread_new_data_index(void);
vtss_addrword_t    vtss_thread_get_data(vtss_thread_key_t index);
void               vtss_thread_set_data(vtss_thread_key_t index, vtss_addrword_t data);
vtss_bool_t        vtss_thread_info_get(vtss_handle_t thread, vtss_thread_info_t *info);

/*---------------------------------------------------------------------------*/
/* Clocks and Alarms                                                         */

// Returns value of real time clock's counter.
vtss_tick_count_t vtss_current_time(void);

class vtss_measure_time
{
public:
    vtss_measure_time();
    vtss_measure_time(const char *s, int l) : start_(vtss_current_time()), s_(s), l_(l)
    {
        printf("%s: %d\n", s, l);
    }
    ~vtss_measure_time()
    {
        printf("%s: %d " VPRI64u " ms\n", s_, l_, vtss_current_time() - start_);
    }
private:
    vtss_tick_count_t start_;
    const char *s_;
    int l_;
};

/*---------------------------------------------------------------------------*/
/* Flags                                                                     */
void              vtss_flag_init         (vtss_flag_t *flag);
void              vtss_flag_destroy      (vtss_flag_t *flag);
void              vtss_flag_setbits      (vtss_flag_t *flag, vtss_flag_value_t value);
void              vtss_flag_maskbits     (vtss_flag_t *flag, vtss_flag_value_t value);
vtss_flag_value_t vtss_flag_wait         (vtss_flag_t *flag, vtss_flag_value_t pattern, vtss_flag_mode_t mode);
vtss_flag_value_t vtss_flag_timed_wait   (vtss_flag_t *flag, vtss_flag_value_t pattern, vtss_flag_mode_t mode, vtss_tick_count_t abstime);
vtss_flag_value_t vtss_flag_timed_waitfor(vtss_flag_t *flag, vtss_flag_value_t pattern, vtss_flag_mode_t mode, vtss_tick_count_t reltime);
vtss_flag_value_t vtss_flag_poll         (vtss_flag_t *flag, vtss_flag_value_t pattern, vtss_flag_mode_t mode);
vtss_flag_value_t vtss_flag_peek         (vtss_flag_t *flag);

/*---------------------------------------------------------------------------*/
/* Semaphores                                                                */
void        vtss_sem_init      (vtss_sem_t *sem, u32 val);
void        vtss_sem_destroy   (vtss_sem_t *sem);
void        vtss_sem_wait      (vtss_sem_t *sem);
vtss_bool_t vtss_sem_trywait   (vtss_sem_t *sem);
vtss_bool_t vtss_sem_timed_wait(vtss_sem_t *sem, vtss_tick_count_t abstime);
void        vtss_sem_post      (vtss_sem_t *sem, u32 increment_by = 1);
u32         vtss_sem_peek      (vtss_sem_t *sem);

/*---------------------------------------------------------------------------*/
/* Mutex                                                                     */
void        vtss_mutex_init       (vtss_mutex_t *mutex);
void        vtss_mutex_destroy    (vtss_mutex_t *mutex);
vtss_bool_t vtss_mutex_lock       (vtss_mutex_t *mutex);
vtss_bool_t vtss_mutex_trylock    (vtss_mutex_t *mutex);
void        vtss_mutex_unlock     (vtss_mutex_t *mutex);

/*---------------------------------------------------------------------------*/
/* Recursive Mutex                                                           */
typedef struct {
    vtss_mutex_t      mutex_;   // Underlying simple mutex
    int               lock_cnt; // Number of locks
} vtss_recursive_mutex_t;

void        vtss_recursive_mutex_init       (vtss_recursive_mutex_t *mutex);
void        vtss_recursive_mutex_destroy    (vtss_recursive_mutex_t *mutex);
vtss_bool_t vtss_recursive_mutex_lock       (vtss_recursive_mutex_t *mutex);
vtss_bool_t vtss_recursive_mutex_trylock    (vtss_recursive_mutex_t *mutex);
int         vtss_recursive_mutex_unlock     (vtss_recursive_mutex_t *mutex);

/*---------------------------------------------------------------------------*/
/* Condition Variables                                                       */
void        vtss_cond_init(vtss_cond_t  *cond, vtss_mutex_t *mutex);
void        vtss_cond_destroy(vtss_cond_t *cond);
vtss_bool_t vtss_cond_wait(vtss_cond_t *cond);
void        vtss_cond_signal(vtss_cond_t *cond);
void        vtss_cond_broadcast(vtss_cond_t *cond);
vtss_bool_t vtss_cond_timed_wait(vtss_cond_t *cond, vtss_tick_count_t abstime);

/*---------------------------------------------------------------------------*/
/* Flash                                                                     */
#define VTSS_FS_FLASH_DIR               "/switch/"
#define VTSS_FS_FILE_DIR                "/switch/icfg/"
#define VTSS_FS_RUN_DIR                 "/var/run/"

#define VTSS_CONF_FILE_NAME_LEN_MAX     256 /* dirent d_name length */
#define VTSS_ICONF_FILE_NAME_LEN_MAX    VTSS_CONF_FILE_NAME_LEN_MAX
#define VTSS_ICFG_PATH                  VTSS_FS_FILE_DIR

int    vtss_flash_get_info(u32 devno, vtss_flash_info_t *info);
int    vtss_flash_read(const vtss_flashaddr_t flash_base, void *ram_base, size_t len, vtss_flashaddr_t *err_address);
int    vtss_flash_erase(vtss_flashaddr_t flash_base, size_t len, vtss_flashaddr_t *err_address);
int    vtss_flash_program(vtss_flashaddr_t flash_base, const void *ram_base, size_t len, vtss_flashaddr_t *err_address);
const char *vtss_flash_errmsg(const int err);

/*---------------------------------------------------------------------------*/
/* CRC                                                                       */
u32 vtss_crc32(const unsigned char *s, int len);
u32 vtss_crc32_accumulate(u32 crc, const unsigned char *s, int len);

// Encode a buffer into base64 format.
// In/Out : to     - Destination buffer.
// In     : to_len - The length of the "to" buffer
//        : from   - Source buffer-
//          len    - Amount of data to be encoded.
// Return : VTSS_RC_OK if encoding was done correct, else error code.
mesa_rc vtss_httpd_base64_encode(char *to, size_t to_len, const char *from, size_t len);

// Decode a buffer into base64 format.
// In/Out : to    - Destination buffer.
// In     : to_len - The length of the "to" buffer
//        : from  - Source buffer-
//          len   - Amount of data to be encoded.
// Return : VTSS_RC_OK if decoding was done correct, else error code.
mesa_rc vtss_httpd_base64_decode(char *to, size_t to_len, const char *from, size_t len);

// Calculate hash value with SHA1.
// In     : input_str     - The input string.
// In     : input_str_len - The length of input string.
// Out    : output_bytes  - The output of hash value (hex bytes).
// Return : VTSS_RC_OK if the process was done correct, else error code.
mesa_rc vtss_sha1_calc(u8 *input_str, u32 input_str_len, u8 output_bytes[20]);

// Calculate hash value with SHA256.
// In     : input_str     - The input string.
// In     : input_str_len - The length of input string.
// Out    : output_bytes  - The output of hash value (hex bytes).
// Return : VTSS_RC_OK if the process was done correct, else error code.
mesa_rc vtss_sha256_calc(u8 *input_str, u32 input_str_len, u8 output_bytes[32]);

// Generate a random hex string.
// In     : byte_num - The number of hex byte.
// Out    : output  - The output of random hex string.
//          Notice the output buffer size should great than (2 * byte_num + 1)
// Return : VTSS_RC_OK if the process was done correct, else error code.
mesa_rc vtss_generate_random_hex_str(u32 byte_num, char *output);

// Encrypt a clear text with AES256.
// In     : input_str     - The input string.
// In     : key           - The key for encryption.
// In     : key_size      - The key size (bytes).
// In     : output_str_buf_len - The buffer length for output text.
// Out    : output_str    - The output of encrypted value (hex bytes).
// Return : VTSS_RC_OK if the process was done correct, else error code.
mesa_rc vtss_aes256_encrypt(char *input_str, u8 *key, u32 key_size, u32 output_str_buf_len, char *output_str);

// Decrypt an AES256 hex value.
// In     : input_hex_str      - The encrypted hex string.
// In     : key                - The key for decryption.
// In     : key_size           - The key size (bytes).
// In     : output_str_buf_len - The buffer length for output text.
// Out    : output_str         - The output text after decryption.
// Return : VTSS_RC_OK if the process was done correct, else error code.
mesa_rc vtss_aes256_decrypt(char input_hex_str[256], u8 *key, u32 key_size, u32 output_str_buf_len, char *output_str);

#ifdef __cplusplus
}
#endif

#endif /* __VTSS_OS_WRAPPER_H__ */
