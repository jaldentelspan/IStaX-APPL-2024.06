#!/usr/bin/env bash
#/*
#
#   Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.
#
#   Unpublished rights reserved under the copyright laws of the United States of
#   America, other countries and international treaties. Permission to use, copy,
#   store and modify, the software and its source code is granted but only in
#   connection with products utilizing the Microsemi switch and PHY products.
#   Permission is also granted for you to integrate into other products, disclose,
#   transmit and distribute the software only in an absolute machine readable
#   format (e.g. HEX file) and only in or with products utilizing the Microsemi
#   switch and PHY products.  The source code of the software may not be
#   disclosed, transmitted or distributed without the prior written permission of
#   Microsemi.
#
#   This copyright notice must appear in any copy, modification, disclosure,
#   transmission or distribution of the software.  Microsemi retains all
#   ownership, copyright, trade secret and proprietary rights in the software and
#   its source code, including all modifications thereto.
#
#   THIS SOFTWARE HAS BEEN PROVIDED "AS IS". MICROSEMI HEREBY DISCLAIMS ALL
#   WARRANTIES OF ANY KIND WITH RESPECT TO THE SOFTWARE, WHETHER SUCH WARRANTIES
#   ARE EXPRESS, IMPLIED, STATUTORY OR OTHERWISE INCLUDING, WITHOUT LIMITATION,
#   WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR USE OR PURPOSE AND
#   NON-INFRINGEMENT.
#
# */

set -x
frr_dir=$HOME
usage="$(basename "$0") [-h] [--frr=<path>] -- 
where:
    -h           show this help text
    --frr=<dir>  set the FRR directory; ex: --frr=/tmp/frr"

while :; do
	case $1 in
		-h|-\?|--help)
			echo "$usage"
			exit
			;;
		--frr=?*)
			frr_dir=${1#*=}
			;;
		*)               # Default case:
			break
	esac
	shift
done
#
#  0. Clean up and Set parameter
#
#export 'PS4=+(${LINENO})'  #turn on to show line number

declare -a reqfiles=($frr_dir/sbin/zebra $frr_dir/sbin/ospfd \
                $frr_dir/etc/zebra.conf.sample $frr_dir/etc/ospfd.conf.sample \
				$frr_dir/etc/vtysh.conf);
for item in "${reqfiles[@]}"
do
if [ ! -f "$item" ]; then
	echo $item "does not exist...exit"
	exit 1
fi
done
echo "frr_dir is" $frr_dir


killall zebra
killall ospfd
ip -all netns delete
#mkdir -p /tmp

#
#  1. How many virutal network devices needed
#
VirtualNetworkDevices='red blue green yellow white brown black grey'
for dev in $VirtualNetworkDevices; do 
	rm -rf /tmp/$dev
	mkdir /tmp/$dev
	# Add name spaces
	ip netns add $dev
done

#
#  2. How many virutal links needed
#
# Add links
for i in {1..8}; do
ip link add name link${i}_a type veth peer name link${i}_b
done

#
#  3. Depict the topology 
#
# Connect links to name spaces
ip link set link1_a netns red
ip link set link1_b netns green
ip link set link2_a netns red
ip link set link2_b netns blue
ip link set link3_a netns green
ip link set link3_b netns brown
ip link set link4_a netns blue
ip link set link4_b netns yellow
ip link set link5_a netns yellow
ip link set link5_b netns brown
ip link set link6_a netns red
ip link set link6_b netns white
ip link set link7_a netns brown
ip link set link7_b netns black
ip link set link8_a netns brown
ip link set link8_b netns grey


#
#  4. Run the zebra and ospfd and Configure the OSPF in each virtual device  
#
# configure ns red - a virtual machine running ospf/Linux
ip netns exec red ip link set lo up
ip netns exec red ip addr add 192.168.1.1/24 dev link1_a
ip netns exec red ip link set link1_a up
ip netns exec red ip addr add 192.168.2.1/24 dev link2_a
ip netns exec red ip link set link2_a up
ip netns exec red ip addr add 10.0.3.1/24 dev link6_a
ip netns exec red ip link set link6_a up

ip netns exec red $frr_dir/sbin/zebra -d -z /tmp/zebra.red.socket -i /tmp/zebra.red.pid \
                                      -f $frr_dir/etc/zebra.conf.sample --vty_socket /tmp/red
ip netns exec red $frr_dir/sbin/ospfd -d -z /tmp/zebra.red.socket -i /tmp/ospf.red.pid  \
                                      -f $frr_dir/etc/ospfd.conf.sample --vty_socket /tmp/red
ip netns exec red $frr_dir/bin/vtysh --vty_socket /tmp/red -c "configure terminal" -c "ip forwarding"
ip netns exec red $frr_dir/bin/vtysh --vty_socket /tmp/red -c "configure terminal" -c "router ospf" \
                                     -c "network 192.168.1.0/24 area 0.0.0.0"
ip netns exec red $frr_dir/bin/vtysh --vty_socket /tmp/red -c "configure terminal" -c "router ospf" \
                                     -c "network 192.168.2.0/24 area 0.0.0.0"
ip netns exec red $frr_dir/bin/vtysh --vty_socket /tmp/red -c "configure terminal" -c "router ospf" \
                                     -c "network 10.0.3.0/24 area 0.0.0.0"

# configure ns white - a virtaul machine running Linux
ip netns exec white ip link set lo up
ip netns exec white ip addr add 10.0.3.2/24 dev link6_b
ip netns exec white ip link set link6_b up
ip netns exec white route add -net 0.0.0.0 netmask 0.0.0.0 gw 10.0.3.1 dev link6_b


# configure ns blue - a virtual machine running ospf/Linux
ip netns exec blue ip link set lo up
ip netns exec blue ip addr add 192.168.2.2/24 dev link2_b
ip netns exec blue ip link set link2_b up
ip netns exec blue ip addr add 192.168.4.1/24 dev link4_a
ip netns exec blue ip link set link4_a up
ip netns exec blue $frr_dir/sbin/zebra -d -z /tmp/zebra.blue.socket -i /tmp/zebra.blue.pid -f $frr_dir/etc/zebra.conf.sample --vty_socket /tmp/blue
ip netns exec blue $frr_dir/sbin/ospfd -d -z /tmp/zebra.blue.socket -i /tmp/ospf.blue.pid  -f $frr_dir/etc/ospfd.conf.sample --vty_socket /tmp/blue
ip netns exec blue $frr_dir/bin/vtysh --vty_socket /tmp/blue -c "configure terminal" -c "ip forwarding"
ip netns exec blue $frr_dir/bin/vtysh --vty_socket /tmp/blue -c "configure terminal" -c "router ospf" -c "network 192.168.2.0/24 area 0.0.0.0"
ip netns exec blue $frr_dir/bin/vtysh --vty_socket /tmp/blue -c "configure terminal" -c "router ospf" -c "network 192.168.4.0/24 area 0.0.0.0"

# configure ns green - a virtual machine running ospf/Linux
ip netns exec green ip link set lo up
ip netns exec green ip addr add 192.168.1.2/24 dev link1_b
ip netns exec green ip link set link1_b up
ip netns exec green ip addr add 192.168.3.1/24 dev link3_a
ip netns exec green ip link set link3_a up
ip netns exec green $frr_dir/sbin/zebra -d -z /tmp/zebra.green.socket -i /tmp/zebra.green.pid -f $frr_dir/etc/zebra.conf.sample --vty_socket /tmp/green
ip netns exec green $frr_dir/sbin/ospfd -d -z /tmp/zebra.green.socket -i /tmp/ospf.green.pid  -f $frr_dir/etc/ospfd.conf.sample --vty_socket /tmp/green
ip netns exec green $frr_dir/bin/vtysh --vty_socket /tmp/green -c "configure terminal" -c "ip forwarding"
ip netns exec green $frr_dir/bin/vtysh --vty_socket /tmp/green -c "configure terminal" -c "router ospf" -c "network 192.168.1.0/24 area 0.0.0.0"
ip netns exec green $frr_dir/bin/vtysh --vty_socket /tmp/green -c "configure terminal" -c "router ospf" -c "network 192.168.3.0/24 area 0.0.0.0"


# configure ns yellow - a virtual machine running ospf/Linux
ip netns exec yellow ip link set lo up
ip netns exec yellow ip addr add 192.168.4.2/24 dev link4_b
ip netns exec yellow ip link set link4_b up
ip netns exec yellow ip addr add 192.168.5.2/24 dev link5_a
ip netns exec yellow ip link set link5_a up

ip netns exec yellow $frr_dir/sbin/zebra -d -z /tmp/zebra.yellow.socket -i /tmp/zebra.yellow.pid -f $frr_dir/etc/zebra.conf.sample --vty_socket /tmp/yellow
ip netns exec yellow $frr_dir/sbin/ospfd -d -z /tmp/zebra.yellow.socket -i /tmp/ospf.yellow.pid  -f $frr_dir/etc/ospfd.conf.sample --vty_socket /tmp/yellow
ip netns exec yellow $frr_dir/bin/vtysh --vty_socket /tmp/yellow -c "configure terminal" -c "ip forwarding"
ip netns exec yellow $frr_dir/bin/vtysh --vty_socket /tmp/yellow -c "configure terminal" -c "router ospf" -c "network 192.168.4.0/24 area 0.0.0.0"
ip netns exec yellow $frr_dir/bin/vtysh --vty_socket /tmp/yellow -c "configure terminal" -c "router ospf" -c "network 192.168.5.0/24 area 0.0.0.0"

# configure ns brown - a virtual machine running ospf/Linux
ip netns exec brown ip link set lo up
ip netns exec brown ip addr add 192.168.3.2/24 dev link3_b
ip netns exec brown ip link set link3_b up
ip netns exec brown ip addr add 192.168.5.1/24 dev link5_b
ip netns exec brown ip link set link5_b up
ip netns exec brown ip addr add 10.0.1.1/24 dev link7_a
ip netns exec brown ip link set link7_a up
ip netns exec brown ip addr add 10.0.2.1/24 dev link8_a
ip netns exec brown ip link set link8_a up

ip netns exec brown $frr_dir/sbin/zebra -d -z /tmp/zebra.brown.socket -i /tmp/zebra.brown.pid -f $frr_dir/etc/zebra.conf.sample --vty_socket /tmp/brown
ip netns exec brown $frr_dir/sbin/ospfd -d -z /tmp/zebra.brown.socket -i /tmp/ospf.brown.pid  -f $frr_dir/etc/ospfd.conf.sample --vty_socket /tmp/brown
ip netns exec brown $frr_dir/bin/vtysh --vty_socket /tmp/brown -c "configure terminal" -c "ip forwarding"
ip netns exec brown $frr_dir/bin/vtysh --vty_socket /tmp/brown -c "configure terminal" -c "router ospf" -c "network 192.168.5.0/24 area 0.0.0.0"
ip netns exec brown $frr_dir/bin/vtysh --vty_socket /tmp/brown -c "configure terminal" -c "router ospf" -c "network 192.168.3.0/24 area 0.0.0.0"
ip netns exec brown $frr_dir/bin/vtysh --vty_socket /tmp/brown -c "configure terminal" -c "router ospf" -c "network 10.0.1.0/24 area 0.0.0.0"
ip netns exec brown $frr_dir/bin/vtysh --vty_socket /tmp/brown -c "configure terminal" -c "router ospf" -c "network 10.0.2.0/24 area 0.0.0.0"


# configure ns black - a virtaul machine running Linux
ip netns exec black ip link set lo up
ip netns exec black ip addr add 10.0.1.2/24 dev link7_b
ip netns exec black ip link set link7_b up
ip netns exec black route add -net 0.0.0.0 netmask 0.0.0.0 gw 10.0.1.1 dev link7_b

# configure ns grey - a virtaul machine running Linux
ip netns exec grey ip link set lo up
ip netns exec grey ip addr add 10.0.2.2/24 dev link8_b
ip netns exec grey ip link set link8_b up
ip netns exec grey route add -net 0.0.0.0 netmask 0.0.0.0 gw 10.0.2.1 dev link8_b


#
# 5. Test
#
# check routing table for each ns
ip netns exec red $frr_dir/bin/vtysh --vty_socket /tmp/red -c "show ip route"
ip netns exec green $frr_dir/bin/vtysh --vty_socket /tmp/green -c "show ip route"
ip netns exec blue $frr_dir/bin/vtysh --vty_socket /tmp/blue -c "show ip route"
ip netns exec yellow $frr_dir/bin/vtysh --vty_socket /tmp/yellow -c "show ip route"
ip netns exec brown $frr_dir/bin/vtysh --vty_socket /tmp/brown -c "show ip route"

#check the ospf command output
ip netns exec yellow $frr_dir/bin/vtysh --vty_socket /tmp/yellow -c "show ip ospf neigh"
ip netns exec yellow $frr_dir/bin/vtysh --vty_socket /tmp/yellow -c "show ip ospf neigh detail"
ip netns exec yellow $frr_dir/bin/vtysh --vty_socket /tmp/yellow -c "show ip ospf interface"
ip netns exec yellow $frr_dir/bin/vtysh --vty_socket /tmp/yellow -c "show ip ospf interface link5_a"
ip netns exec yellow $frr_dir/bin/vtysh --vty_socket /tmp/yellow -c "show ip ospf"
ip netns exec yellow $frr_dir/bin/vtysh --vty_socket /tmp/yellow -c "show ip ospf database"

# ping from ns black to ns white and vice versa
#ip netns exec black ping 10.0.3.2
#ip netns exec white ping 10.0.1.2

# print the kernel routing table for each ns
#ip netns exec yellow route
#ip netns exec green route
#ip netns exec blue route
#ip netns exec red route	

#ip netns exec blue bash
#export PS1="blue \u@\h:\w\$ "

#ip netns exec red bash
#export PS1="red \u@\h:\w\$ "

