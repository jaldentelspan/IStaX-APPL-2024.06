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

#ifndef __VTSS_BASICS_FD_HXX__
#define __VTSS_BASICS_FD_HXX__

#include "vtss/basics/string.hxx"

namespace vtss {

// This class encapsulate a generic file descriptor, and represents the
// ownership of the file descriptor. The file-descriptor can be moved but not
// copied, it can release the owner ship and it can be assigned with a new file
// descriptor. This class uses RAII to ensure that the file descriptor is always
// closed.
struct Fd {
    // Default construct a Fd class, without owning a file-descriptor.
    constexpr Fd() : fd_(-1){};

    // Construct a Fd class, and assign the ownership of 'fd'.
    constexpr Fd(int fd) : fd_(fd){};

    // Move constructor. Moves the ownership of the file descriptor from 'rhs'
    // to this class.
    Fd(Fd &&rhs) {
        fd_ = rhs.fd_;
        rhs.fd_ = -1;
    }

    // Move assignment. Moves the ownership of the file descriptor from 'rhs' to
    // this class.
    Fd &operator=(Fd &&rhs) {
        (void)close();
        fd_ = rhs.fd_;
        rhs.fd_ = -1;
        return *this;
    }

    // Coping not allowed
    Fd(const Fd &rhs) = delete;
    Fd &operator=(const Fd &rhs) = delete;

    // Destructor - ensure that file-descriptors is always closed.
    ~Fd() { (void)close(); }

    // Assign the ownership of a 'raw' file-descriptor.
    void assign(int fd) {
        (void)close();
        fd_ = fd;
    }

    // Release the ownership of a file-descriptor.
    int release() {
        int tmp = fd_;
        fd_ = -1;
        return tmp;
    }

    // Close the file-descriptor. Return the 0 on success, otherwise return the
    // value of 'errno'.
    int close();

    // Get the raw file-descriptor without releasing it.
    int raw() const { return fd_; }

  private:
    int fd_;
};

FixedBuffer read_file_into_buf(const char *f, size_t grow = 4096,
                               size_t max = (size_t)-1);
ssize_t write_buf_into_file(const char* buff, size_t size, const char *f);


}  // namespace vtss

#endif  // __VTSS_BASICS_FD_HXX__
