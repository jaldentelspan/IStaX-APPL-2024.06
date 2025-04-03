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

#ifndef _VTSS_FIRMWARE_H_
#define _VTSS_FIRMWARE_H_

#include "firmware_api.h"
#include "download.hxx"
#include "vtss_module_id.h"
#include "control_api.h" /* For mesa_restart_t */
#include "vtss_md5_api.h"    /* md5* */

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_FIRMWARE

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_FIRMWARE

#define VTSS_TRACE_GRP_DEFAULT 0
#define VTSS_TRACE_GRP_INFO    1

#include <vtss_trace_api.h>
/* ============== */

/* Firmware update */
typedef struct {
    cli_iolayer_t        *io;
    const unsigned char  *buffer;       /* Firmware buffer (malloc) */
    char                 filename[FIRMWARE_IMAGE_NAME_MAX];  /* Name of file uploaded */
    size_t               length;        /* malloc'ed bytes */
    vtss_bool_t          sync;          /* Signal completion through request (no reboot) */
    vtss_sem_t           sem;           /* Synchronization element */
    mesa_rc              rc;            /* Async reply */
    unsigned long        image_version; /* Currently: 0 = image uses fixed flash layout, 1 = image uses FIS to get flash layout, falls back to fixed flash layout */
    u32                  chiptype;      /* Chip target - VSCXXXX */
    u8                   cputype;       /* CPU Target - ARM=1, MIPS=2 */
    mesa_restart_t       restart;       /* How to reboot when done uploading */
} firmware_flash_args_t;

/*
 * Firmware image trailer format
 */
typedef struct firmware_trailer {
    /* Common part - all versions */
    unsigned long magic;
#define FIRMWARE_TRAILER_MAGIC 0xadfacade
    unsigned long crc;          /* Signed hash/auth key */
    /* Auth key omitted in trailer data */
#define FIRMWARE_TRAILER_AUTHKEY 0xead34bc3
    unsigned long version;
    /* Version dependant part */
#define FIRMWARE_TRAILER_VER_1 1
    struct {
        unsigned long type;     /* Image type */
#define FIRMWARE_TRAILER_V1_TYPE_MANAGED   1
#define FIRMWARE_TRAILER_V1_TYPE_UNMANAGED 2 /* Compressed */
        unsigned long flags;
#define FIRMWARE_TRAILER_V1_FLAG_GZIP     (1 << 0)
        unsigned long image_version;
        unsigned long cpu_product_chiptype;
        unsigned long spare;
    } v1;
} firmware_trailer_t;

/* ================================================================= *
 *  Firmware state
 * ================================================================= */

/* Block versions */
#define FIRMWARE_STATE_BLK_VERSION 1

typedef struct {
    u8 signature[MD5_MAC_LEN];
} firmware_state_t;

/* Firmware metadata block */
typedef struct {
    u32          version;        /* Block version */
    firmware_state_t data;       /* Firmware metadata */
} firmware_state_blk_t;

/* ================================================================= *
 *  Port stack messages
 * ================================================================= */

/* Main thread mail message types */
typedef enum {
    FIRMWARE_MBMSG_CHECKGO = 0xcafe, /* Check whether all switches are ready */
    /* (void *) = */
} firmware_mbmsg_t;

/* Port messages IDs */
typedef enum {
    FIRMWARE_MSG_ID_IMAGE_CNF,   /* Image received, ready */
    FIRMWARE_MSG_ID_IMAGE_BURN,  /* Start flash update    */
    FIRMWARE_MSG_ID_IMAGE_ABRT,  /* Abort flash update    */
    FIRMWARE_MSG_ID_IMAGE_BEGIN, /* Begin firmware update */
} firmware_msg_id_t;

typedef struct firmware_msg {
    firmware_msg_id_t msg_id;
    char              filename[FIRMWARE_IMAGE_NAME_MAX];
    mesa_restart_t    restart;
} firmware_msg_t;

enum {
    _CPU_TYPE_ARM  = 1,
    _CPU_TYPE_MIPS = 2,
    _CPU_TYPE_AARCH64 = 3,
    _CPU_TYPE_UNKNOWN = 999,
};

#if defined(__ARMEL__)
#define MY_CPU_ARC  _CPU_TYPE_ARM
#elif defined (__MIPSEL__)
#define MY_CPU_ARC  _CPU_TYPE_MIPS
#elif defined (__AARCH64EL__)
#define MY_CPU_ARC _CPU_TYPE_AARCH64
#else
#error "Please port code to this target CPU type"
#define MY_CPU_ARC  _CPU_TYPE_UNKNOWN
#endif

#include "vtss_mtd_api.hxx"

#define VTSS_REDBOOT_RFIS_VALID_MAGIC_LENGTH 10
#define VTSS_REDBOOT_RFIS_VALID_MAGIC ".FisValid"  // exactly 10 bytes

#define VTSS_REDBOOT_RFIS_VALID       (0xa5)       // this FIS table is valid, the only "good" value
#define VTSS_REDBOOT_RFIS_IN_PROGRESS (0xfd)       // this FIS table is being modified
#define VTSS_REDBOOT_RFIS_EMPTY       (0xff)       // this FIS table is empty

typedef struct {
    char magic_name[VTSS_REDBOOT_RFIS_VALID_MAGIC_LENGTH];
    unsigned char valid_flag[2];  // one of the flags defined above
    uint32_t version_count;  // with each successfull update the version count will increase by 1
} fis_valid_info;

typedef struct {
    union
    {
        char name[16];  /* Null terminated name */
        fis_valid_info valid_info;
    } u;
    uint32_t flash_base;
    uint32_t mem_base;
    uint32_t size;
    uint32_t entry_point;
    uint32_t data_length;
    unsigned char _pad[256 - (16 + 4*sizeof(uint32_t) + 3*sizeof(uint32_t))];
    uint32_t desc_cksum;
    uint32_t file_cksum;
} vtss_fis_desc_t;

typedef struct {
    const char *name;// FIS/mtd name
    u8 *fis_data;    // Malloc'ed FIS directory
    u32 fis_size;    // Size of data
    vtss_fis_desc_t *fis_start, *fis_end;    // Start and end of the above
    vtss_mtd_t mtd;
} vtss_fis_mtd_t;

mesa_rc vtss_fis_open(vtss_fis_mtd_t *fis_mtd, const char *name);
mesa_rc vtss_fis_close(vtss_fis_mtd_t *fis_mtd);
const char* firmware_cmdline_get();
const char *firmware_fis_act(void);
const char *firmware_fis_prim(void);
const char *firmware_fis_bak(void);
BOOL firmware_has_alt(void);
const char *firmware_fis_to_update(void);
const char *firmware_fis_layout(void);

BOOL firmware_mtd_checksame(cli_iolayer_t *io, vtss_mtd_t *mtd, const char *name, const u8 *buffer, size_t length);
bool has_part_name(FILE *fp, const char *partname);
const char *get_mmc_device(const char *partname);

//mesa_rc firmware_download(const char *url, FirmwareDownload &dld, bool cli_io);

void firmware_init_os(vtss_init_data_t *data);

typedef enum {
    FWS_BUSY,
    FWS_ERASING,
    FWS_ERASED,
    FWS_PROGRAMMING,
    FWS_PROGRAMMED,
    FWS_SWAPPING,
    FWS_SWAP_DONE,
    FWS_SWAP_FAILED,
    FWS_PROGRAM_FAILED,
    FWS_ERASE_FAILED,
    FWS_SAME_IMAGE,
    FWS_INVALID_IMAGE,
    FWS_UNKNOWN_MTD,
    FWS_REBOOT,
    FWS_DONE,
} fw_state_t;

void firmware_setstate(cli_iolayer_t *io, fw_state_t state, const char *name);

#endif /* _VTSS_FIRMWARE_H_ */

