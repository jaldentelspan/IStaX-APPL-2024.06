#!/bin/sh
#
# Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.
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

if [ $# -ne 2 ]; then
echo "Please call with arguments ip-address and MIB prefix. e.g.:"
echo "./getmibs.sh 10.10.133.223 VTSS"
exit
fi

HOST=$1
SMILINT_FLAGS="-l 6 -c ./smirc"
PREFIX=$2

fetch_mib() {
    echo "Fetching $PREFIX-$1-MIB.mib"
    if curl -sSf -u admin: http://$HOST/$PREFIX-$1-MIB.mib -o $PREFIX-$1-MIB.mib
    then
        if [ -f $PREFIX-$1-MIB.mib ]
        then
            smilint $SMILINT_FLAGS $PREFIX-$1-MIB.mib
            ./vtss-mib-lint.rb $PREFIX-$1-MIB.mib > /dev/null
        fi
    fi
}

fetch_mib "ACCESS-MANAGEMENT"
fetch_mib "ACL"
fetch_mib "ALARM"
fetch_mib "AGGR"
fetch_mib "APS"
fetch_mib "ARP-INSPECTION"
fetch_mib "AUTH"
fetch_mib "DAYLIGHT-SAVING"
fetch_mib "DDMI"
fetch_mib "DHCP-RELAY"
fetch_mib "DHCP-SERVER"
fetch_mib "DHCP-SNOOPING"
fetch_mib "DHCP6-CLIENT"
fetch_mib "DHCP6-RELAY"
fetch_mib "DHCP6-SNOOPING"
fetch_mib "DNS"
fetch_mib "EEE"
fetch_mib "ERPS"
fetch_mib "FAN"
fetch_mib "FIRMWARE"
fetch_mib "FRER"
fetch_mib "GVRP"
fetch_mib "HTTPS"
fetch_mib "ICFG"
fetch_mib "IECMRP"
fetch_mib "IP"
fetch_mib "IP-SOURCE-GUARD"
fetch_mib "IPMC-MVR"
fetch_mib "IPMC-PROFILE"
fetch_mib "IPMC-SNOOPING"
fetch_mib "IPV6-SOURCE-GUARD"
fetch_mib "JSON-RPC-NOTIFICATION"
fetch_mib "LACP"
fetch_mib "LED-POWER-REDUCTION"
fetch_mib "LLDP"
fetch_mib "LOOP-PROTECTION"
fetch_mib "MAC"
fetch_mib "MACSEC"
fetch_mib "MIRROR"
fetch_mib "MRP"
fetch_mib "MSTP"
fetch_mib "MVRP"
fetch_mib "NAS"
fetch_mib "NTP"
fetch_mib "OSPF"
fetch_mib "OSPF6"
fetch_mib "POE"
fetch_mib "PORT"
fetch_mib "PORT-POWER-SAVINGS"
fetch_mib "PRIVILEGE"
fetch_mib "PSEC"
fetch_mib "PSFP"
fetch_mib "PTP"
fetch_mib "PVLAN"
fetch_mib "QOS"
fetch_mib "REDBOX"
fetch_mib "RIP"
fetch_mib "ROUTER"
fetch_mib "SNMP"
fetch_mib "SSH"
fetch_mib "STREAM"
fetch_mib "SYNCE"
fetch_mib "SYSLOG"
fetch_mib "SYSUTIL"
fetch_mib "THERMAL-PROTECTION"
fetch_mib "UDLD"
fetch_mib "UPNP"
fetch_mib "USERS"
fetch_mib "VCL"
fetch_mib "VLAN"
fetch_mib "VLAN-TRANSLATION"
fetch_mib "VOICE-VLAN"

