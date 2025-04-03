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
#/bin/sh

echoerr() { echo "$@" 1>&2; }

D=$(mktemp -d)
DD=$D/vtss
INC_MSCC=$D/mscc

echo $D

mkdir $DD
mkdir $INC_MSCC

cp -r $API_BUILD_PATH/include_common/mscc $D/.
cp -r ../../vtss_appl/include/vtss/appl $DD/.
cp -r ../../vtss_basics/include/vtss/basics $DD/.
cp -r ../../vtss_basics/platform/linux/include/vtss/basics/* $DD/basics/.

cd $D

RES=0

for i in $(find . -type f \( -name "*.hxx" -or -name "*.h" \) -not -path "./vtss/basics/*" | sort)
do

    if [ $i == "./mscc/ethernet/switch/api/hdr_end.h" ]
    then
        continue;
    fi

    if [ $i == "./mscc/ethernet/switch/api/hdr_start.h" ]
    then
        continue;
    fi

    if [ $i == "./vtss/appl/optional_modules_create.hxx" ]
    then
        continue;
    fi

    OUT=$(g++ "$@" -std=c++17 -DVTSS_BASICS_STANDALONE -I$D $i 2>&1 )
    if [ $? -eq "0" ]
    then
        echo "Testing public header $i - OK"
    else
        RES=1
        echoerr "Testing public header $i - ERROR!"
        echo g++ "$@" -std=c++17 -DVTSS_BASICS_STANDALONE -I$D $i
        g++ "$@" -std=c++17 -DVTSS_BASICS_STANDALONE -I$D $i
        echoerr
    fi
done

rm -rf $D

exit $RES

