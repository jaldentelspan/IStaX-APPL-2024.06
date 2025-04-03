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

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/random.h>
#include <sys/ioctl.h>
#include "mfi.h"

typedef enum
{
    false,
    true
} bool;

void basic_linux_system_init_urandom(const char *SEED) {
    bool seed_ok = false;
    const size_t buf_size = 512;
    static const char *DEV = "/dev/urandom";
    // static const char *SEED = /* VTSS_FS_FLASH_DIR */ "/random-seed";
    struct rand_pool_info *ent;
    uint32_t *buf;
    uint32_t *p;
    size_t s;
    int fd = -1;

    if ((ent = (struct rand_pool_info*) malloc(sizeof(*ent)+buf_size)) == NULL) {
        warn("malloc fails: %zd bytes\n", sizeof(*ent)+buf_size);
        return;
    }
    buf = &ent->buf[0];

    // Read the seed from last time (if it exists)
    seed_ok = false;
    fd = open(SEED, O_RDONLY);
    if (fd != -1) {
        p = buf;
        s = buf_size;

        while (s) {
            int res = read(fd, buf, s);
            if (res == -1) {
                warn("URANDOM: Failed to read from %s: %s\n", SEED,
                     strerror(errno));
                break;
            } else if (res == 0) {
                // zero indicates end of file
                break;
            } else {
                p += res;
                s -= res;
            }
        }

        if (s == 0) {
            seed_ok = true;
        }

        close(fd);
    }

    // Write the seed into the urandom device
    if (seed_ok) {
        fd = open(DEV, O_WRONLY);
        if (fd != -1) {
            ent->buf_size = buf_size;
            ent->entropy_count = ent->buf_size << 3;    // 8 bits entropy per byte (100%)
            if(ioctl(fd, RNDADDENTROPY, ent)) {
                warn("ioctl(%s): %s", "RNDADDENTROPY\n", strerror(errno));
            } else {
                warn("Added %d bytes of entropy to %s\n", ent->entropy_count, DEV);
            }
            close(fd);
        } else {
            warn("open(%s) fails: %s\n", DEV, strerror(errno));
        }
    }

    // Read a new seed from urandom
    seed_ok = false;
    fd = open(DEV, O_RDONLY);
    if (fd != -1) {
        p = buf;
        s = buf_size;

        while (s) {
            int res = read(fd, buf, s);
            if (res == -1) {
                warn("URANDOM: Failed to read from %s: %s\n", DEV, strerror(errno));
                break;
            } else {
                p += res;
                s -= res;
            }
        }

        if (s == 0) {
            seed_ok = true;
        }

        close(fd);
    } else {
        warn("URANDOM: Failed to open %s: %s\n", DEV, strerror(errno));
    }

    // Write the seed such that it will be used on the next boot
    if (seed_ok) {
        fd = open(SEED, O_WRONLY | O_CREAT | O_TRUNC);
        if (fd != -1) {
            p = buf;
            s = buf_size;

            while (s) {
                int res = write(fd, buf, s);
                if (res == -1) {
                    warn("URANDOM: Failed to write to %s: %s\n", SEED,
                         strerror(errno));
                    break;
                } else {
                    p += res;
                    s -= res;
                }
            }

            close(fd);

        } else {
            warn("URANDOM: Failed to open %s: %s\n", SEED, strerror(errno));
        }
    }
    free(ent);
}

