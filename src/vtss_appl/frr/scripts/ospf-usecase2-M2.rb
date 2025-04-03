#!/usr/bin/env ruby
#   Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.
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

# Microchip is aware that some terminology used in this technical document is
# antiquated and inappropriate. As a result of the complex nature of software
# where seemingly simple changes have unpredictable, and often far-reaching
# negative results on the software's functionality (requiring extensive retesting
# and revalidation) we are unable to make the desired changes in all legacy
# systems without compromising our product or our clients' products.

require "pp"

# specify the one device not to emulate (because we want to hook it up to a real
# board)
$dut_unit = "r3"

$frr_dir = "/usr/local"

# Specify how to hook-up the $dut_unit
$dut_eth_map_r1 = { "b0.2_r1"  => {:local_nic => "eth_blue",   :dut_if => "gi 1/1",  :dut_vlan_if => "100"},
                 "b0.4_r1"  => {:local_nic => "eth_yellow", :dut_if => "gi 1/2",  :dut_vlan_if => "200"},
                 "b1.10_r1"  => {:local_nic => "eth_brown", :dut_if => "gi 1/3",  :dut_vlan_if => "300"},
                 "b1.11_r1" => {:local_nic => "eth_red",    :dut_if => "gi 1/4",  :dut_vlan_if => "400"} }
$dut_eth_map_r2 = { "b0.1_r2"  => {:local_nic => "eth_blue",   :dut_if => "gi 1/1",  :dut_vlan_if => "100"},
                 "b0.2_r2"  => {:local_nic => "eth_yellow", :dut_if => "gi 1/2",  :dut_vlan_if => "200"},
                 "b2.5_r2"  => {:local_nic => "eth_brown", :dut_if => "gi 1/3",  :dut_vlan_if => "300"},
                 "b2.6_r2" => {:local_nic => "eth_red",    :dut_if => "gi 1/4",  :dut_vlan_if => "400"} }
$dut_eth_map_r3 = { "b0.1_r3"  => {:local_nic => "eth1",   :dut_if => "gi 1/1",  :dut_vlan_if => "100"},
                 "b0.3_r3"  => {:local_nic => "eth2", :dut_if => "gi 1/2",  :dut_vlan_if => "200"},
                 "b3.12_r3" => {:local_nic => "enx000242bc6625",    :dut_if => "gi 1/3",  :dut_vlan_if => "300"} }
$dut_eth_map_r4 = { "b0.3_r4"  => {:local_nic => "eth_blue",   :dut_if => "gi 1/1",  :dut_vlan_if => "100"},
                 "b0.4_r4"  => {:local_nic => "eth_yellow", :dut_if => "gi 1/2",  :dut_vlan_if => "200"},
                 "b4.14_r4"  => {:local_nic => "eth_brown", :dut_if => "gi 1/3",  :dut_vlan_if => "300"},
                 "b4.15_r4" => {:local_nic => "eth_red",    :dut_if => "gi 1/4",  :dut_vlan_if => "400"} }

$dut_eth_map = $dut_eth_map_r3

$networks = {}
$units = {}
$links = {}
$dut_link_map = {}

def run cmd, flags = []
    o = %x{#{cmd} 2>&1}
    puts "run: #{cmd}" if not flags.include?(:silence)
    raise "\nCMD Failed with exit code: #{$?}\n#{cmd}\n#{o}\n" if $?.to_i != 0
    return o
end

def sys cmd
    system cmd
    raise "\nCMD Failed: >#{cmd}< with exit code: #{$?}\n" if $?.to_i != 0
end

def background cmd, cond_files = []
    puts "Background: #{cmd}"
    pid = spawn(cmd)
    Process.detach(pid)

    start = Time.now.to_i
    cond_files.each do |f|
        until File.exist?(f)
            sleep 0.1
            raise "Time is out" if Time.now.to_i - start > 10
        end
    end
end

def try cmd
    o = %x{#{cmd} 2>&1}
    if  $?.to_i != 0
        puts "try (failed): #{cmd}"
    else
        puts "try: #{cmd}"
    end
    return o
end

def get_link_name a, b
    base = "#{a}_#{b}"
    if $links[base].nil?
        $links[base] = 1
        return base
    end

    idx = 0
    loop do
        l = "#{base}_#{idx}"
        if $links[l].nil?
            $links[l] = 1
            return l
        end
        idx += 1
    end
end

def id_from_name n
    if /\w+(\d+)/ =~ n
        return $1
    end
    raise "no match"
end

def add_network network, units
    raise "Network created already" if not $networks[network].nil?

    $networks[network] = units

    puts "Adding network: 1.#{network}.x with units: [#{units.join(", ")}]"

    units.each do |u|
        if $units[u].nil? and $dut_unit != u
            run "ip netns add ns_#{u}"
            run "ip netns exec ns_#{u} ip link set dev lo up"
            $units[u] = 1
        end
    end

    case units.size
    when 0
    when 1
    when 2
        # We need a veth-pair
        u0 = units[0]
        u1 = units[1]
        l = get_link_name u0, u1

        if $dut_unit == u0
            $dut_link_map[l] = { :ns => "ns_#{u1}",
                                 :local_nic_addr => "1.#{network}.#{id_from_name(u1)}/24",
                                 :dut_addr => "1.#{network}.#{id_from_name(u0)}/24" }
        elsif $dut_unit == u1
            $dut_link_map[l] = { :ns => "ns_#{u0}",
                                 :local_nic_addr => "1.#{network}.#{id_from_name(u0)}/24",
                                 :dut_addr => "1.#{network}.#{id_from_name(u1)}/24" }
        else
            run "ip link add #{l}_a type veth peer name #{l}_b"
            run "ip link set #{l}_a netns ns_#{u0}"
            run "ip link set #{l}_b netns ns_#{u1}"

            run "ip netns exec ns_#{u0} ip link set dev #{l}_a up"
            run "ip netns exec ns_#{u0} ip addr add 1.#{network}.#{id_from_name(u0)}/24 dev #{l}_a"

            run "ip netns exec ns_#{u1} ip link set dev #{l}_b up"
            run "ip netns exec ns_#{u1} ip addr add 1.#{network}.#{id_from_name(u1)}/24 dev #{l}_b"
        end

    else
        # We need a bridge
        br = "b#{network}"

        if $units["br"].nil?
            run "ip netns add ns_br"
            run "ip netns exec ns_br ip link set dev lo up"
            $units["br"] = 1
        end

        run "ip netns exec ns_br ip link add #{br} type bridge"
        run "ip netns exec ns_br ip link set #{br} up"

        units.each do |u|
            l = get_link_name br, u

            if $dut_unit == u
                $dut_link_map[l] = { :ns => "ns_br",
                                     :master_dev => br,
                                     :dut_addr => "1.#{network}.#{id_from_name(u)}/24" }
            else
                run "ip link add #{l}_a type veth peer name #{l}_b"
                run "ip link set #{l}_a netns ns_#{u}"
                run "ip link set #{l}_b netns ns_br"

                run "ip netns exec ns_#{u} ip link set dev #{l}_a up"
                run "ip netns exec ns_#{u} ip addr add 1.#{network}.#{id_from_name(u)}/24 dev #{l}_a"

                run "ip netns exec ns_br ip link set dev #{l}_b master #{br}"
                run "ip netns exec ns_br ip link set dev #{l}_b up"
            end
        end
    end

    puts ""
end

def map_phys_links
    err = 0
    $dut_link_map.each do |k, v|
        if $dut_eth_map[k].nil?
            puts "Missing physical-to-logical for #{k}"
            err += 1
        else
            $dut_link_map[k][:local_nic]   = $dut_eth_map[k][:local_nic]
            $dut_link_map[k][:dut_if]      = $dut_eth_map[k][:dut_if]
            $dut_link_map[k][:dut_vlan_if] = $dut_eth_map[k][:dut_vlan_if]
        end
    end

    if err != 0
        raise
    end

    pp $dut_link_map

    $dut_link_map.each do |k, v|
        run "ip link set dev #{v[:local_nic]} down"
        run "ip addr flush dev #{v[:local_nic]}"
        run "ip link set #{v[:local_nic]} netns #{v[:ns]}"
        if not v[:master_dev].nil?
            run "ip netns exec #{v[:ns]} ip link set dev #{v[:local_nic]} master #{v[:master_dev]}"
        end
        if v[:local_nic_addr]
            run "ip netns exec #{v[:ns]} ip addr add #{v[:local_nic_addr]} dev #{v[:local_nic]}"
        end
        run "ip netns exec #{v[:ns]} ip link set dev #{v[:local_nic]} up"
    end

end

def start_frr
    $units.each do |k, v|
        next if $dut_unit == k
        next if not /r\d+/ =~ k
        puts "Start FRR for #{k}"

        run "rm -rf /tmp/#{k} /tmp/zebra.#{k}.socket /tmp/zebra.#{k}.pid /tmp/ospf.#{k}.pid"
        run "mkdir -p /tmp/#{k}"

        background("ip netns exec ns_#{k} #{$frr_dir}/sbin/zebra -u root -g root -d -z /tmp/zebra.#{k}.socket -i /tmp/zebra.#{k}.pid -f #{$frr_dir}/etc/zebra.conf.sample --vty_socket /tmp/#{k}",
                   ["/tmp/zebra.#{k}.socket", "/tmp/zebra.#{k}.pid", "/tmp/#{k}/zebra.vty"])

        background("ip netns exec ns_#{k} #{$frr_dir}/sbin/ospfd -u root -g root -d -z /tmp/zebra.#{k}.socket -i /tmp/ospf.#{k}.pid  -f #{$frr_dir}/etc/ospfd.conf.sample --vty_socket /tmp/#{k}",
                   ["/tmp/ospf.#{k}.pid", "/tmp/#{k}/ospfd.vty"])
    end
end

def def_route ns, network, gw
       g = id_from_name gw
       run "ip netns exec ns_#{ns} route add default gw 1.#{network}.#{g}"
end

def vty_raw ns, cmds
    if ns == $dut_unit
        puts "SKIPPING #{ns} [#{cmds.map{|x| "\"#{x}\""}.join(", ")}]"
        return
    end
    run "ip netns exec ns_#{ns} #{$frr_dir}/bin/vtysh --vty_socket /tmp/#{ns} #{cmds.map{|x| "-c \"#{x}\""}.join("  ")}"
end

def vty_conf ns, cmd
    vty_raw ns, ["configure terminal", cmd]
end

def vty_conf_ospf ns, cmd
    vty_raw ns, ["configure terminal", "router ospf", cmd]
end

def vty_st ns, cmd
    vty_raw ns, [cmd]
end

try "killall vtysh"
try "killall ospfd"
try "killall zebra"
try "ip -a netns del"

add_network "0.2",  ["r1",  "r2", "h104"]
def_route "h104", "0.2", "r1"
add_network "0.1",  ["r2",  "r3", "h101"]
def_route "h101", "0.1", "r2"
add_network "0.3",  ["r3",  "r4", "h102"]
def_route "h102", "0.3", "r3"
add_network "0.4",  ["r4",  "r1", "h103"]
def_route "h103", "0.4", "r4"

add_network "1.10", ["r1",  "r14", "h113"]
def_route "h113", "1.10", "r1"
add_network "1.11", ["r13", "r1",  "h117"]
def_route "h117", "1.11", "r3"
add_network "1.7",  ["r12", "r13", "h116"]
def_route "h116", "1.7", "r12"
add_network "1.8",  ["r15", "r12", "h114"]
def_route "h114", "1.8", "r15"
add_network "1.9",  ["r14", "r15", "h112"]
def_route "h112", "1.9", "r14"

add_network "2.5",  ["r21", "r2",  "h108"]
def_route "h108", "2.5", "r21"
add_network "2.6",  ["r2",  "r21", "h107"]
def_route "h107", "2.6", "r2"

add_network "3.12", ["r31", "r3", "h106"]
def_route "h106", "3.12", "r31"
#def_route "h202", "3.12", "r31"
#def_route "h204", "3.12", "r3"
#def_route "h205", "3.12", "r3"


add_network "4.15", ["r41", "r4",  "h113"]
def_route "h113", "4.15", "r41"
add_network "4.14", ["r42", "r4",  "h112"]
def_route "h112", "4.14", "r42"

add_network "5.16", ["r41", "h110"]
def_route "h110", "5.16", "r41"
add_network "0.17", ["r42", "h109"]
def_route "h109", "0.17", "r42"
add_network "99.1", ["r15", "h118"]
def_route "h118", "99.1", "r15"

map_phys_links()

start_frr()

puts "Configure routes"
vty_conf      "r1",  "ip forwarding"
vty_conf_ospf "r1",  "network 1.0.2.0/24 area 0.0.0.0"
vty_conf_ospf "r1",  "network 1.0.4.0/24 area 0.0.0.0"
vty_conf_ospf "r1",  "network 1.1.10.0/24 area 0.0.0.1"
vty_conf_ospf "r1",  "network 1.1.11.0/24 area 0.0.0.1"
vty_conf_ospf "r1",  "router-id 0.0.0.1"

vty_conf      "r2",  "ip forwarding"
vty_conf_ospf "r2",  "network 1.0.2.0/24 area 0.0.0.0"
vty_conf_ospf "r2",  "network 1.0.1.0/24 area 0.0.0.0"
vty_conf_ospf "r2",  "network 1.2.5.0/24 area 0.0.0.2"
vty_conf_ospf "r2",  "network 1.2.6.0/24 area 0.0.0.2"
vty_conf_ospf "r2",  "area 0.0.0.2 stub no-summary"
vty_conf_ospf "r2",  "router-id 0.0.0.2"

vty_conf      "r3",  "ip forwarding"
vty_conf_ospf "r3",  "network 1.0.1.0/24 area 0.0.0.0"
vty_conf_ospf "r3",  "network 1.0.3.0/24 area 0.0.0.0"
vty_conf_ospf "r3",  "network 1.3.12.0/24 area 0.0.0.3"
vty_conf_ospf "r3",  "area 0.0.0.3 stub"
vty_conf_ospf "r3",  "router-id 0.0.0.3"

vty_conf      "r4",  "ip forwarding"
vty_conf_ospf "r4",  "network 1.0.4.0/24 area 0.0.0.0"
vty_conf_ospf "r4",  "network 1.0.3.0/24 area 0.0.0.0"
vty_conf_ospf "r4",  "network 1.4.14.0/24 area 0.0.0.4"
vty_conf_ospf "r4",  "network 1.4.15.0/24 area 0.0.0.4"
vty_conf_ospf "r4",  "area 4 virtual-link 0.0.0.41"
vty_conf_ospf "r4",  "area 4 virtual-link 0.0.0.42"
vty_conf_ospf "r4",  "router-id 0.0.0.4"

vty_conf "r12", "ip forwarding"
vty_conf_ospf "r12",  "network 1.1.7.0/24 area 0.0.0.1"
vty_conf_ospf "r12",  "network 1.1.8.0/24 area 0.0.0.1"
vty_conf_ospf "r12",  "router-id 0.0.0.12"

vty_conf "r13", "ip forwarding"
vty_conf_ospf "r13",  "network 1.1.7.0/24 area 0.0.0.1"
vty_conf_ospf "r13",  "network 1.1.11.0/24 area 0.0.0.1"
vty_conf_ospf "r13",  "router-id 0.0.0.13"

vty_conf "r14", "ip forwarding"
vty_conf_ospf "r14",  "network 1.1.9.0/24 area 0.0.0.1"
vty_conf_ospf "r14",  "network 1.1.10.0/24 area 0.0.0.1"
vty_conf_ospf "r14",  "router-id 0.0.0.14"

vty_conf "r15", "ip forwarding"
vty_conf_ospf "r15",  "network 1.1.8.0/24 area 0.0.0.1"
vty_conf_ospf "r15",  "network 1.1.9.0/24 area 0.0.0.1"
vty_conf_ospf "r15",  "redistribute connected"
vty_conf_ospf "r15",  "router-id 0.0.0.15"

vty_conf "r21", "ip forwarding"
vty_conf_ospf "r21",  "network 1.2.5.0/24 area 0.0.0.2"
vty_conf_ospf "r21",  "network 1.2.6.0/24 area 0.0.0.2"
vty_conf_ospf "r21",  "area 0.0.0.2 stub"
vty_conf_ospf "r21",  "router-id 0.0.0.21"

vty_conf "r31", "ip forwarding"
vty_conf_ospf "r31",  "network 1.3.12.0/24 area 0.0.0.3"
vty_conf_ospf "r31",  "router-id 0.0.0.31"
vty_conf_ospf "r31",  "area 0.0.0.3 stub"

vty_conf "r41", "ip forwarding"
vty_conf_ospf "r41",  "network 1.4.15.0/24 area 0.0.0.4"
vty_conf_ospf "r41",  "network 1.5.16.0/24 area 0.0.0.5"
vty_conf_ospf "r41",  "area 0.0.0.4 virtual-link 0.0.0.4"
vty_conf_ospf "r41",  "router-id 0.0.0.41"


vty_conf "r42", "ip forwarding"
vty_conf_ospf "r42",  "network 1.4.14.0/24 area 0.0.0.4"
vty_conf_ospf "r42",  "network 1.0.17.0/24 area 0.0.0.0"
vty_conf_ospf "r42",  "area 0.0.0.4 virtual-link 0.0.0.4"
vty_conf_ospf "r42",  "router-id 0.0.0.42"

pp $dut_link_map

