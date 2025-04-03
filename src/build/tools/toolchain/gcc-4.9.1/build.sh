#!/bin/sh
#
# Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.
#
# Unpublished rights reserved under the copyright laws of the United States of
# America, other countries and international treaties. Permission to use, copy,
# store and modify, the software and its source code is granted but only in
# connection with products utilizing the Microsemi switch and PHY products.
# Permission is also granted for you to integrate into other products, disclose,
# transmit and distribute the software only in an absolute machine readable format
# (e.g. HEX file) and only in or with products utilizing the Microsemi switch and
# PHY products.  The source code of the software may not be disclosed, transmitted
# or distributed without the prior written permission of Microsemi.
#
# This copyright notice must appear in any copy, modification, disclosure,
# transmission or distribution of the software.  Microsemi retains all ownership,
# copyright, trade secret and proprietary rights in the software and its source code,
# including all modifications thereto.
#
# THIS SOFTWARE HAS BEEN PROVIDED "AS IS". MICROSEMI HEREBY DISCLAIMS ALL WARRANTIES
# OF ANY KIND WITH RESPECT TO THE SOFTWARE, WHETHER SUCH WARRANTIES ARE EXPRESS,
# IMPLIED, STATUTORY OR OTHERWISE INCLUDING, WITHOUT LIMITATION, WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR USE OR PURPOSE AND NON-INFRINGEMENT.
#

set -e -x

# Downlaod crosstool
wget http://crosstool-ng.org/download/crosstool-ng/crosstool-ng-1.20.0.tar.bz2

# Extract it
tar -xjf crosstool-ng-1.20.0.tar.bz2
cd crosstool-ng-1.20.0

# Patch it to enable eabi
patch -p1 < ../ct-ng-eabi.patch

# configure and build it
./configure --enable-local
make

# copy the configuration which matches the vtss-mips CPU
cp ../ct-ng.config .config
./ct-ng oldconfig

# build the toolchain
./ct-ng build

# copy the resulting toolchain
mv vtss-cross-mips32-24kec vtss-cross-ecos-mips32-24kec-v2
tar -cvj vtss-cross-ecos-mips32-24kec-v2 -f vtss-cross-ecos-mips32-24kec-v2.tar.bz2
mv vtss-cross-ecos-mips32-24kec-v2.tar.bz2 ../.

# clean up
cd ..
chmod -R +w crosstool-ng-1.20.0
rm crosstool-ng-1.20.0.tar.bz2
#rm -rf crosstool-ng-1.20.0

