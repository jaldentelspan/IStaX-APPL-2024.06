#!/bin/sh
#
# Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.
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

if [ $# -ne 3 ]; then
echo "Please call with arguments ip-address, MIB prefix and MIB base name. e.g.:"
echo "./getsinglemib.sh 10.10.133.223 VTSS VLAN"
exit
fi

HOST=$1
SMILINT_FLAGS="-l 6 -c ./smirc"
PREFIX=$2
MIBBASENAME=$3

fetch_mib() {
    echo "Fetching $PREFIX-$1-MIB.mib"
    curl -sSf -u admin: http://$HOST/$PREFIX-$1-MIB.mib -o $PREFIX-$1-MIB.mib
    smilint $SMILINT_FLAGS $PREFIX-$1-MIB.mib
    ruby vtss-mib-lint.rb $PREFIX-$1-MIB.mib
}

fetch_mib "$3"
