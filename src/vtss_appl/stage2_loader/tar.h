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

#ifndef __MFI_SRC_TAR_HXX__
#define __MFI_SRC_TAR_HXX__

static const int offset_name = 0;
static const int offset_mode = 100;
static const int offset_size = 124;
static const int offset_type = 156;
static const int offset_linkname = 157;
static const int offset_dev_major = 329;
static const int offset_dev_minor = 337;
static const int offset_content = 512;

enum Type {
    Unknown,
    File,
    HardLink,
    SymbolicLink,
    Device,
    BlockDevice,
    Directory,
    NamedPipe,
    Longname,
    Longlinkname,
};

typedef enum {
    false,
    true
} bool;

typedef struct TarIterator {
    int data_left;
    const char *data;
    const char *longname;
    const char *longlinkname;
} TarIterator;

TarIterator *TarIterator_new(const char *d, int s);

bool tariterator_next(TarIterator *t);
bool tariterator_check(TarIterator *t);

int tariterator_size(TarIterator *t);
int tariterator_mode(TarIterator *t);
enum Type tariterator_type(TarIterator *t);
int tariterator_dev_minor(TarIterator *t);
int tariterator_dev_major(TarIterator *t);
const char *tariterator_content(TarIterator *t);
const char *tariterator_name(TarIterator *t);
const char *tariterator_linkname(TarIterator *t);


int tar_extract(const char *d, size_t s, const char *prefix);

int tar_xz_extract(const char *d, size_t s, const char *prefix);

enum TarError {
    TAR_ERR_FAILED = -1,
    TAR_ERR_AGAIN = -2,
    TAR_ERR_DONE = -3,
};

typedef struct tar_state {
    bool init;
    int fd;

    char name[512];

    char hdr[512];
    unsigned hdr_valid;
    unsigned data_consumed;

} tar_state_t;

int tar_extract_(const char *d, size_t s, const char *prefix);


#endif  // __MFI_SRC_TAR_HXX__
