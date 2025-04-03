#!/bin/sh
#
# Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.
#
# Unpublished rights reserved under the copyright laws of the United States of
# America, other countries and international treaties. Permission to use, copy,
# store and modify, the software and its source code is granted but only in
# connection with products utilizing the Microsemi switch and PHY products.
# Permission is also granted for you to integrate into other products, disclose,
# transmit and distribute the software only in an absolute machine readable
# format (e.g. HEX file) and only in or with products utilizing the Microsemi
# switch and PHY products.  The source code of the software may not be
# disclosed, transmitted or distributed without the prior written permission of
# Microsemi.
#
# This copyright notice must appear in any copy, modification, disclosure,
# transmission or distribution of the software.  Microsemi retains all
# ownership, copyright, trade secret and proprietary rights in the software and
# its source code, including all modifications thereto.
#
# THIS SOFTWARE HAS BEEN PROVIDED "AS IS". MICROSEMI HEREBY DISCLAIMS ALL
# WARRANTIES OF ANY KIND WITH RESPECT TO THE SOFTWARE, WHETHER SUCH WARRANTIES
# ARE EXPRESS, IMPLIED, STATUTORY OR OTHERWISE INCLUDING, WITHOUT LIMITATION,
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR USE OR PURPOSE AND
# NON-INFRINGEMENT.
#

set -e

GCC_VERSION=4.8.2
TOOLCHAIN_INSTALL_PREFIX=/usr/local/vtss/gcc-$GCC_VERSION

export LD_LIBRARY_PATH=$TOOLCHAIN_INSTALL_PREFIX/lib64:$TOOLCHAIN_INSTALL_PREFIX/lib
export PATH=$TOOLCHAIN_INSTALL_PREFIX/bin:$PATH

mkdir -p obj/host/vtss_basics
cd obj/host/vtss_basics
CC=$TOOLCHAIN_INSTALL_PREFIX/bin/gcc                               \
CXX=$TOOLCHAIN_INSTALL_PREFIX/bin/g++                              \
cmake                                                              \
-DBoost_INCLUDE_DIR=/usr/local/vtss/boost-1.55.0-gcc-4.8.2/include \
-DBoost_LIBRARY_DIRS=/usr/local/vtss/boost-1.55.0-gcc-4.8.2/lib    \
../../../../vtss_basics

make -j8
make test

