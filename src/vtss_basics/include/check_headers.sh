#!/bin/sh
#
# Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.
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

echoerr() { echo "$@" 1>&2; }

D=$(mktemp -d)

cp -r vtss $D/.
echo "#define VTSS_SIZEOF_VOID_P 8" > $D/vtss/basics/config.h

cd $D

RES=0

for i in $(find . -type f -name "*.h" -or -name "*.hxx")
do
    echo "Testing public header $i"
    g++ "$@" -std=c++17 -I$D $i
    #OUT=$(g++ "$@" -std=c++17 -I$D $i 2>&1 )
    #if [ $? -eq "0" ]
    #then
    #    echo "Testing public header $i - OK"
    #else
    #    RES=1
    #    echoerr "Testing public header $i - ERROR!"
    #    echoerr $OUT
    #    echoerr
    #fi
done

rm -rf $D

exit $RES

