/*

 Copyright (c) 2006-2018 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "vtss/basics/fd.hxx"

namespace vtss {

int Fd::close() {
    if (fd_ == -1) return EBADF;
    if (::close(fd_) != 0) return errno;
    return 0;
}

FixedBuffer read_file_into_buf(const char *f, size_t grow, size_t max) {
    FixedBuffer buf;
    struct stat st;

    Fd fd(open(f, O_RDONLY));
    if (fd.raw() == -1) {
        return FixedBuffer();
    }

    char *p = buf.begin();
    size_t s_total = 0;
    size_t s_left = 0;

    // Check file size and use that for the initial allocation. Be aware that
    // the returned size for files in /proc and /sys is reported as zero
    // eventhogh they are not empty.
    if (fstat(fd.raw(), &st) == 0 && st.st_size != 0) {
        buf = move(FixedBuffer(min(max, (size_t)st.st_size)));
        s_left = buf.size();
        p = buf.begin();
    }

    while (true) {
        if (s_left == 0) {
            // Calculate new size
            size_t new_size = min(buf.size() + grow, max);

            // Update the new "remaining" bytes
            s_left = new_size - s_total;

            // Check if we have reached the max size
            if (s_left == 0) return FixedBuffer();

            // Allocate a new buffer which is 'grow' bytes larger than the
            // perviouse
            FixedBuffer b(new_size);

            // Check for allocation error
            if (b.size() == 0) return FixedBuffer();

            // Copy the old content
            memcpy(b.begin(), buf.begin(), buf.size());

            // replace buf with 'b'
            buf = move(b);
            p = buf.begin() + s_total;
        }

        int res = read(fd.raw(), p, s_left);
        if (res == -1 || res == 0) {
            break;
        }

        p += res;
        s_total += res;
        s_left -= res;
    }

    // Update size to the amount of valid data
    return FixedBuffer(s_total, buf.release());
}

ssize_t write_buf_into_file(const char* buff, size_t size, const char* f) {
    Fd fd(open(f, O_WRONLY | O_CREAT | O_TRUNC));
    if (fd.raw() == -1) {
        return -1;
    }
    size_t bytes_written = 0;
    while (bytes_written < size) {
        ssize_t res = write(fd.raw(), buff, size);
        if (res <= 0) {
            return res;
        }
        bytes_written += (size_t)res;
    }
    return bytes_written;
}

}  // namespace vtss
