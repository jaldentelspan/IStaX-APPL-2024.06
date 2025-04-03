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
#ifndef __VTSS_OS_WRAPPER_LINUX_H__
#define __VTSS_OS_WRAPPER_LINUX_H__

#include <unistd.h>
#include <stddef.h>

#include "main_types.h"
#include "chrono.hxx"
#include <pthread.h> /* for Posix thread, mutex and condition variables */
#include <signal.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <mqueue.h>
#include <time.h>

/*---------------------------------------------------------------------------*/
/* The following are derived types, they may have different                  */
/* definitions from these depending on configuration.                        */

typedef void      *vtss_addrword_t;     /* May hold pointer or word      */
typedef pthread_t vtss_handle_t;        /* Object handle                 */

typedef u32        vtss_priority_t; // Type for priorities
typedef signed int vtss_code_t;     // Type for various codes
typedef u32        vtss_vector_t;   // Interrupt vector ID
typedef u32        vtss_cpu_t;      // CPU id type
typedef u64        vtss_tick_count_t;
typedef bool       vtss_bool_t;

typedef struct {
    timer_t           tid;
    struct itimerspec its;
    void              (*call_bk)(vtss_handle_t alarm, vtss_addrword_t data);
} vtss_alarm_t;

typedef pthread_mutex_t vtss_mutex_t;

typedef struct {
    pthread_cond_t     cond;
    vtss_mutex_t       *mutex;
} vtss_cond_t;

typedef struct {
    vtss_mutex_t      mutex;
    vtss_cond_t       cond;
    vtss_flag_value_t flags;
} vtss_flag_t;

typedef struct {
    vtss_mutex_t mutex;
    vtss_cond_t  cond;
    u32          count;
} vtss_sem_t;

enum vtss_isr_results {
    VTSS_ISR_HANDLED  = 1, // Interrupt was handled
    VTSS_ISR_CALL_DSR = 2  // Schedule DSR
};

typedef struct {
    unsigned long tcam;
} vtss_post_report_t;

#define vtss_socket(a, b, c) socket(a, b, c)
#define vtss_accept(a, b, c) accept(a, b, c)

/*---------------------------------------------------------------------------*/
typedef pthread_t vtss_thread_t;
typedef pthread_key_t vtss_thread_key_t;

typedef struct {
    vtss_thread_t       handle;
    int                 tid;
    const char          *name;
    vtss_thread_prio_t  prio;
    BOOL                rt;
    int                 os_prio; // If #rt, priority else niceness.
    vtss_addrword_t     stack_base;
    size_t              stack_size;
} vtss_thread_info_t;

// Although the following are defined in vtss_os_wrapper.h, we need it here as well,
// because this file is included first in vtss_os_wrapper.h, and we need it
// for definition of vtss_interrupt_t.
typedef u32  vtss_isr_f(vtss_vector_t vector, vtss_addrword_t data);
typedef void vtss_dsr_f(vtss_vector_t vector, u32 count, vtss_addrword_t data);
typedef struct vtss_interrupt_s {
    vtss_vector_t           vector;
    vtss_priority_t         priority;
    vtss_isr_f              *isr;
    vtss_dsr_f              *dsr;
    vtss_addrword_t         data;
    struct vtss_interrupt_s *volatile next_dsr;
    volatile i32            dsr_count;
} vtss_interrupt_t;

/*---------------------------------------------------------------------------*/
/* Flags                                                                     */
typedef enum {
    VTSS_FLAG_WAITMODE_AND,     /* All bits must be set                       */
    VTSS_FLAG_WAITMODE_OR,      /* Any bit must be set                        */
    VTSS_FLAG_WAITMODE_AND_CLR, /* All bits must be set. Clear when satisfied */
    VTSS_FLAG_WAITMODE_OR_CLR,  /* Any bit must be set. Clear when satisfied  */
} vtss_flag_mode_t;

/*---------------------------------------------------------------------------*/
/* Flash                                                                     */
#define VTSS_FLASH_ERR_OK   VTSS_RC_OK
#define VTSS_FLASH_ERR_GEN  VTSS_RC_ERROR

#define VTSS_FLASH_ERR_INVALID         0x01  // Invalid FLASH address
#define VTSS_FLASH_ERR_ERASE           0x02  // Error trying to erase
#define VTSS_FLASH_ERR_LOCK            0x03  // Error trying to lock/unlock
#define VTSS_FLASH_ERR_PROGRAM         0x04  // Error trying to program
#define VTSS_FLASH_ERR_PROTOCOL        0x05  // Generic error
#define VTSS_FLASH_ERR_PROTECT         0x06  // Device/region is write-protected
#define VTSS_FLASH_ERR_NOT_INIT        0x07  // FLASH info not yet initialized
#define VTSS_FLASH_ERR_HWR             0x08  // Hardware (configuration?) problem
#define VTSS_FLASH_ERR_ERASE_SUSPEND   0x09  // Device is in erase suspend mode
#define VTSS_FLASH_ERR_PROGRAM_SUSPEND 0x0a  // Device is in program suspend mode
#define VTSS_FLASH_ERR_DRV_VERIFY      0x0b  // Driver failed to verify data
#define VTSS_FLASH_ERR_DRV_TIMEOUT     0x0c  // Driver timed out
#define VTSS_FLASH_ERR_DRV_WRONG_PART  0x0d  // Driver does not support device
#define VTSS_FLASH_ERR_LOW_VOLTAGE     0x0e  // Not enough juice to complete job

typedef u8 *vtss_flashaddr_t;
typedef struct vtss_flash_block_info {
    size_t block_size;
    u32    blocks;
} vtss_flash_block_info_t;

typedef struct {
    vtss_flashaddr_t              start;           // First address
    vtss_flashaddr_t              end;             // Last address
    u32                           num_block_infos; // Number of entries
    const vtss_flash_block_info_t *block_info;     // Info about block sizes
} vtss_flash_info_t;

typedef int vtss_flash_printf(const char *fmt, ...);

/*---------------------------------------------------------------------------*/
/* CRC                                                                       */
#define Z_OK            0
#define Z_STREAM_END    1
#define Z_NEED_DICT     2
#define Z_ERRNO         (-1)
#define Z_STREAM_ERROR  (-2)
#define Z_DATA_ERROR    (-3)
#define Z_MEM_ERROR     (-4)
#define Z_BUF_ERROR     (-5)
#define Z_VERSION_ERROR (-6)

#if 0
#define uncompress      z_uncompress
#define compress        z_compress

mesa_rc z_uncompress(uchar *data, ulong *max_size, uchar *p, ulong data_size);
mesa_rc z_compress(uchar *data, ulong *max_size, uchar *p, ulong data_size);
#endif

/*---------------------------------------------------------------------------*/
/* HAL                                                                       */
#ifndef VTSSNUM_HAL_RTC_NUMERATOR
# define VTSSNUM_HAL_RTC_NUMERATOR     1000000000
# define VTSSNUM_HAL_RTC_DENOMINATOR   100
#endif
#define VTSS_MSECS_PER_HWTICK   (VTSSNUM_HAL_RTC_NUMERATOR / VTSSNUM_HAL_RTC_DENOMINATOR / 1000000)

#define VTSS_OS_MSEC2TICK(X) (MAX(1,(X)))
#define VTSS_OS_TICK2MSEC(X)  (X)

#ifdef __cplusplus
extern "C" {
#endif

u64 hal_time_get();

#ifdef __cplusplus
}
#endif

typedef u32 HAL_SavedRegisters;

/****************************************************************************/
// The following macro may be used to calculate the length of the header
// of the user message. The assumption is that the user message is a struct
// with two parts:
//
// 1) Header
// 2) Data
/****************************************************************************/
#define MSG_TX_DATA_HDR_LEN(s, m) offsetof(s, m)

/****************************************************************************/
// The following macro may be used to find the biggest of two header lengths
// of user data messages.
/****************************************************************************/
#define MSG_TX_DATA_HDR_LEN_MAX(s1, m1, s2, m2) (offsetof(s1, m1) > offsetof(s2, m2) ? offsetof(s1, m1) : offsetof(s2, m2))

/* from vtss_api/include/vtss_fdma_api.h file */

#define VTSS_FDMA_HDR_SIZE_BYTES  0 /**< Dummy */

#define VTSS_OS_MSLEEP(X) VTSS_MSLEEP(X)

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

/****************************************************************************/
// TFTP Dummy functions
/****************************************************************************/

#define TFTP_OCTET 1
extern "C"
int tftp_client_get(const char *const filename,
                    const char *const server,
                    const int port,
                    char *buff,
                    int len,
                    const int mode,
                    int *const err);

extern "C"
int tftp_client_put(const char *const filename,
                    const char *const server,
                    const int port,
                    const char *buf,
                    int len,
                    const int mode,
                    int *const err);

// Linux mempool functions
typedef struct vtss_mempool_s {
    int  totalmem;
    int  freemem;
    void *base;
    int  size;
    int  blocksize;
    int  maxfree;   // The largest free block
} vtss_mempool_info_t;

struct vtss_mempool_var_memdq {
    struct vtss_mempool_var_memdq *prev, *next;
    i32    size;
};

typedef struct {
    struct vtss_mempool_var_memdq head;
    u8                            *obase;
    i32                           osize;
    u8                            *bottom;
    u8                            *top;
    i32                           alignment;
    i32                           freemem;
} vtss_mempool_var_t;

typedef struct {
    u32 *bitmap;
    i32 maptop;
    u32 *mempool;
    i32 numblocks;
    i32 freeblocks;
    i32 blocksize;
    i32 firstfree;
    u8  *top;
} vtss_mempool_fix_t;

/* Create a variable size memory pool */
extern "C"
void vtss_mempool_var_create(void               *base,   // Base of memory to use for pool
                             i32                size,    // Size of memory in bytes
                             vtss_handle_t      *handle, // Returned handle of memory pool
                             vtss_mempool_var_t *var);   // Space to put pool structure in

/* Allocates a block of length size. NULL is returned if no memory is available. */
void *vtss_mempool_var_try_alloc(vtss_handle_t varpool, i32 size);

/* Frees memory back into variable size pool. */
void vtss_mempool_var_free(vtss_handle_t varpool, void *p);

/* Puts information about a variable memory pool into the structure provided. */
void vtss_mempool_var_get_info(vtss_handle_t varpool, vtss_mempool_info_t *info);

/* Create a fixed size memory pool */
void vtss_mempool_fix_create(void               *base,     // Base of memory to use for pool
                             i32                size,      // Size of memory in byte
                             i32                blocksize, // Size of allocation in bytes
                             vtss_handle_t      *handle,   // Handle of memory pool
                             vtss_mempool_fix_t *fix);     // Space to put pool structure in

/* Allocates a block.  NULL is returned if no memory is available. */
void *vtss_mempool_fix_try_alloc(vtss_handle_t fixpool);

/* Frees memory back into fixed size pool. */
void vtss_mempool_fix_free(vtss_handle_t fixpool, void *p);

/* Puts information about a variable memory pool into the structure provided. */
void vtss_mempool_fix_get_info(vtss_handle_t fixpool, vtss_mempool_info_t *info);

/****************************************************************************/
// HAL_TABLE
/****************************************************************************/
#define CYG_HAL_TABLE_TYPE

// NPI/IFH interface naming
#define VTSS_NPI_DEVICE "vtss.ifh"

/*---------------------------------------------------------------------------*/
/* Serial IO                                                                 */
typedef int vtss_io_handle_t;

// Supported baud rates
typedef enum {
    VTSS_SERIAL_BAUD_50 = 1,
    VTSS_SERIAL_BAUD_75,
    VTSS_SERIAL_BAUD_110,
    VTSS_SERIAL_BAUD_134_5,
    VTSS_SERIAL_BAUD_150,
    VTSS_SERIAL_BAUD_200,
    VTSS_SERIAL_BAUD_300,
    VTSS_SERIAL_BAUD_600,
    VTSS_SERIAL_BAUD_1200,
    VTSS_SERIAL_BAUD_1800,
    VTSS_SERIAL_BAUD_2400,
    VTSS_SERIAL_BAUD_3600,
    VTSS_SERIAL_BAUD_4800,
    VTSS_SERIAL_BAUD_7200,
    VTSS_SERIAL_BAUD_9600,
    VTSS_SERIAL_BAUD_14400,
    VTSS_SERIAL_BAUD_19200,
    VTSS_SERIAL_BAUD_38400,
    VTSS_SERIAL_BAUD_57600,
    VTSS_SERIAL_BAUD_115200,
    VTSS_SERIAL_BAUD_230400,
    VTSS_SERIAL_BAUD_460800,
    VTSS_SERIAL_BAUD_921600
} vtss_serial_baud_rate_t;

// Stop bit selections
typedef enum {
    VTSS_SERIAL_STOP_1 = 1,
    VTSS_SERIAL_STOP_2
} vtss_serial_stop_bits_t;

// Parity modes
typedef enum {
    VTSS_SERIAL_PARITY_NONE = 0,
    VTSS_SERIAL_PARITY_EVEN,
    VTSS_SERIAL_PARITY_ODD,
    VTSS_SERIAL_PARITY_MARK,
    VTSS_SERIAL_PARITY_SPACE
} vtss_serial_parity_t;

// Word length
typedef enum {
    VTSS_SERIAL_WORD_LENGTH_5 = 5,
    VTSS_SERIAL_WORD_LENGTH_6,
    VTSS_SERIAL_WORD_LENGTH_7,
    VTSS_SERIAL_WORD_LENGTH_8
} vtss_serial_word_length_t;

typedef struct {
    vtss_serial_baud_rate_t baud;
    vtss_serial_stop_bits_t stop;
    vtss_serial_parity_t parity;
    vtss_serial_word_length_t word_length;
    u32 flags;
} vtss_serial_info_t;

// receive flow control, send RTS when necessary:
#define VTSS_SERIAL_FLOW_RTSCTS_RX         (1<<2)  // FIXME: The value has been copied from ECOS implementation. It may not be appropriate for Linux.
// transmit flow control, act when not CTS:
#define VTSS_SERIAL_FLOW_RTSCTS_TX         (1<<3)  // FIXME: The value has been copied from ECOS implementation. It may not be appropriate for Linux.

#define ENOERR VTSS_RC_OK

#define VTSS_OS_RS422                   "/dev/ttyS1"

#define VTSS_IO_SET_CONFIG_SERIAL_INFO                  0x0181 // FIXME: The value has been copied from ECOS implementation. It may not be appropriate for Linux.
#define VTSS_IO_GET_CONFIG_SERIAL_INFO                  0x0101 // FIXME: The value has been copied from ECOS implementation. It may not be appropriate for Linux.

// Lookup a device and return it's handle
mesa_rc vtss_io_lookup(const char *name, vtss_io_handle_t *handle);

// Write data to a device
mesa_rc vtss_io_write(vtss_io_handle_t handle, const void *buf, size_t *len);

// Read data from a device
mesa_rc vtss_io_read(vtss_io_handle_t handle, void *buf, size_t *len);

// Set config for device
mesa_rc vtss_io_set_config(vtss_io_handle_t handle, u32 key, const void *buf, size_t *len);

// Read config for device
mesa_rc vtss_io_get_config(vtss_io_handle_t handle, u32 key, void *buf, size_t *len);

// Generate random number
mesa_rc vtss_random(uint32_t *p);

#endif /* __VTSS_OS_WRAPPER_LINUX_H__ */
