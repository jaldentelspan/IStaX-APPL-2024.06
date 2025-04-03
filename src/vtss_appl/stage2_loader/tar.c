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

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sysmacros.h>

#include "tar.h"
#include "mfi.h"
#include <linux/xz.h>

static int read_octet(const char *p, int size) {
    int res = 0, i;

    for (i = 0; i < size; ++i, ++p) {
        if (*p == 0) continue;
        res = res * 8 + *p - '0';
    }

    return res;
}

TarIterator *tariterator_new(const char *d, int s) {
    TarIterator *t = (TarIterator *)malloc(sizeof(TarIterator));

    if (t) {
        t->data_left = s;
        t->data = d;
        t->longname = NULL;
        t->longlinkname = NULL;
    }
    return t;
}

bool tariterator_check(TarIterator *t) {
    if (t->data_left < offset_content) {
        debug("TAR: Not enough data: %d %d\n", t->data_left, offset_content);
        return false;
    }

    // Check for EOF
    if (strnlen(tariterator_name(t), 2) == 0) return false;

    if (t->longname) {
        int s = strnlen(tariterator_name(t), 512);
        if (s <= 0 || s >= 512) {
            debug("TAR: Unexpected name length: %d %s\n", s, tariterator_name(t));
            return false;
        }
    } else {
        int s = strnlen(tariterator_name(t), 100);
        if (s <= 0 || s >= 100) {
            debug("TAR: Unexpected name length: %d %s\n", s, tariterator_name(t));
            return false;
        }
    }

    if (tariterator_size(t) + offset_content > t->data_left) {
        debug("TAR: Not enough data: %d %d\n", tariterator_size(t) + offset_content, t->data_left);
        return false;
    }

    return true;
}

const char *tariterator_name(TarIterator *t) {
    if (t->longname)
        return t->longname;
    else
        return t->data + offset_name;
}

int tariterator_mode(TarIterator *t) {
    return read_octet(t->data + offset_mode, 7) & 07777;
}

int tariterator_dev_minor(TarIterator *t) {
    return read_octet(t->data + offset_dev_minor, 7);
}

int tariterator_dev_major(TarIterator *t) {
    return read_octet(t->data + offset_dev_major, 7);
}

const char *tariterator_linkname(TarIterator *t) {
    if (t->longlinkname)
        return t->longlinkname;
    else
        return t->data + offset_linkname;
}

int tariterator_size(TarIterator *t) { return read_octet(t->data + offset_size, 11); }

const char *tariterator_content(TarIterator *t) { return t->data + offset_content; }

enum Type tariterator_type(TarIterator *t) {
    char c = *(t->data + offset_type);
    switch (c) {
    case '0':  // Fallthrough
    case '\0':
        return File;
    case '1':
        return HardLink;
    case '2':
        return SymbolicLink;
    case '3':
        return Device;
    case '4':
        return BlockDevice;
    case '5':
        return Directory;
    case '6':
        return NamedPipe;
    case 'L':
        return Longname;
    case 'K':
        return Longlinkname;
    }

    debug("TAR: Unsupported type: %c(%d)\n", c, (int)c);
    return Unknown;
}

bool tariterator_next(TarIterator *t) {
    int s = tariterator_size(t);
    int r = s % offset_content;
    if (r != 0) r = offset_content - r;
    s += r;
    s += offset_content;

    if (tariterator_type(t) == Longname) {
        t->longname = tariterator_content(t);
    } else {
        t->longname = NULL;
    }

    if (tariterator_type(t) == Longlinkname) {
        t->longlinkname = tariterator_content(t);
    } else {
        t->longlinkname = NULL;
    }

    t->data += s;
    t->data_left -= s;

    return tariterator_check(t);
}

static int write_file(const char *n, const char *d, size_t s, int mode) {
    int fd = open(n, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (fd < 0) {
        warn("Could not open file: %s\n", strerror(errno));
        return -1;
    }

    while (s) {
        int res = write(fd, d, s);
        if (res < 0) {
            warn("Could not write to file: %s\n", strerror(errno));
            close(fd);
            return -1;
        }

        s -= res;
        d += res;
    }

    close(fd);
    return 0;
}

int tar_extract(const char *d, size_t s, const char *prefix) {
    const int name_length = 512;
    int res;
    char name[name_length];
    TarIterator *i = NULL;

    res = mkdir(prefix, 0777);
    if (res == -1 && errno != EEXIST) {
        warn("Failed to create prefix directory: %s\n", strerror(errno));
        goto do_exit;
    }

    debug("Extracting tar archive of size: %u to %s\n", (int)s, prefix);
    i = tariterator_new(d, s);

    if (!tariterator_check(i)) {
        debug("Content does not look like a tar file\n");
        res = -1;
        goto do_exit;
    }

    do {
        snprintf(name, name_length, "%s/%s", prefix, tariterator_name(i));
        name[name_length - 1] = 0;

        switch (tariterator_type(i)) {
        case File:
            noise("File:        %06o %s %d bytes\n", tariterator_mode(i), name, tariterator_size(i));
            res = write_file(name, tariterator_content(i), tariterator_size(i), tariterator_mode(i));
            if (res < 0) goto do_exit;
            break;

        case SymbolicLink:
            noise("Symlink:     %s -> %s\n", name, tariterator_linkname(i));
            unlink(name);  // remove it if it already exists
            res = symlink(tariterator_linkname(i), name);
            if (res == -1) {
                warn("Failed to create symlink: %s\n", strerror(errno));
                goto do_exit;
            }
            break;

        case Directory:
            noise("Directory:   %06o %s\n", tariterator_mode(i), name);
            res = mkdir(name, tariterator_mode(i));
            if (res == -1 && errno != EEXIST) {
                warn("Failed to create directory: %s\n", strerror(errno));
                goto do_exit;
            }
            break;

        case Device:
            res = mknod(name, S_IFCHR | tariterator_mode(i),
                        makedev(tariterator_dev_minor(i), tariterator_dev_major(i)));
            if (res == -1) {
                warn("Failed to create char-device: %s\n", strerror(errno));
                goto do_exit;
            }
            break;

        case BlockDevice:
            res = mknod(name, S_IFBLK | tariterator_mode(i),
                        makedev(tariterator_dev_minor(i), tariterator_dev_major(i)));
            if (res == -1) {
                warn("Failed to create block-device: %s\n", strerror(errno));
                goto do_exit;
            }
            break;

        case Longname:
        case Longlinkname:
            break;

        default:
            warn("Unsupported type: %d\n", (int)tariterator_type(i));
        }
    } while (tariterator_next(i));

    res = 0;

do_exit:
    if (i) {
        free(i);
    }
    return res;
}

int tar_xz_extract(const char *d, size_t s, const char *prefix) {
    int res = 0;
    struct xz_dec *xz_state = xz_dec_init(XZ_DYNALLOC, 1 << 26);
    struct xz_buf buf = {};
    buf.in = (const unsigned char*)d;
    buf.in_pos = 0;
    buf.in_size = s;

    if (!xz_state) {
        warn("Out of memory");
        res = -1;
        goto do_exit;
    }

    xz_crc32_init();

    buf.out_pos = 0;
    buf.out_size = s * 20;
    buf.out = (unsigned char*)malloc(buf.out_size); // rely on lazy allocation
    if (!buf.out) {
        warn("Out of memory");
        xz_dec_end(xz_state);
        res = -1;
        goto do_exit;
    }

    res = xz_dec_run(xz_state, &buf);
    switch (res) {
        case XZ_OK:
        case XZ_STREAM_END:
            break;

#define CASE(X) \
        case X: \
            warn("XZ-error: " #X " (%d)\n", (int)res); \
            res = -1; \
            goto END;

        CASE(XZ_UNSUPPORTED_CHECK);
        CASE(XZ_MEM_ERROR);
        CASE(XZ_MEMLIMIT_ERROR);
        CASE(XZ_FORMAT_ERROR);
        CASE(XZ_OPTIONS_ERROR);
        CASE(XZ_DATA_ERROR);
        CASE(XZ_BUF_ERROR);

        default:
            warn("XZ-error:  UNKNOWN (%d)\n", (int)res);
            goto END;
    }

    res = tar_extract((const char *)buf.out, buf.out_pos, prefix);

END:
    xz_dec_end(xz_state);
    free(buf.out);

do_exit:
    return res;
}

