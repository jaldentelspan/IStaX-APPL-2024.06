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

#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdlib.h>
#include <zlib.h>
#include <string.h>
#include <inttypes.h>

#define LOG_ERROR 2
#define LOG_WARN  4
#define LOG_INFO  6
#define LOG_DEBUG 7
#define LOG_NOISE 8

extern unsigned int g_log_level;
extern char g_switch_profile[64];

int warn(const char *fmt, ...);
int info(const char *fmt, ...);
int debug(const char *fmt, ...);
int noise(const char *fmt, ...);

#define DIR_STAGE2        "/tmp/stage2"
#define DIR_ROOT_ELEM     DIR_STAGE2 "/rootfs-elements"
#define DIR_ROOT_ELEM_IX  DIR_ROOT_ELEM "/%d"
#define DIR_NEW_ROOT      DIR_STAGE2 "/new-root"

typedef struct Buf {
    size_t size;
    char *data;
} Buf;

int rootfs_extract(const char *d, size_t s, const char *prefix);
int process_stage2_fd(int fd, int fd_loop, size_t off, size_t len);

#endif  // __MAIN_H__
