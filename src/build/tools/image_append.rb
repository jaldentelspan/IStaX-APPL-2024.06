#!/usr/bin/env ruby
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

# Microchip is aware that some terminology used in this technical document is
# antiquated and inappropriate. As a result of the complex nature of software
# where seemingly simple changes have unpredictable, and often far-reaching
# negative results on the software's functionality (requiring extensive retesting
# and revalidation) we are unable to make the desired changes in all legacy
# systems without compromising our product or our clients' products.

require 'pp'
require 'optparse'

def sys(cmd)
    if !ENV["V"].nil?
        puts cmd
    end
    system(cmd)
    if $? != 0
        STDERR.puts("Error: Execution of '#{cmd}' for '#{$mfi_file}' failed: #{$?}")
        exit(-1)
    end
end

# Adding Uboot env-file
# The U-Boot environment size in the flash is
# a hard-coded value and should match the U-boot
# configuration under include/configs/vcoreiii.h
# #define CONFIG_ENV_SIZE             (8 * 1024)
# #define CONFIG_ENV_SECT_SIZE      (256 * 1024)
# Furthermore, note that we have support for two
# flash types, one with 64K sector size and one
# with 256K sector size. However, for simplicity
# reasons, we use the greatest common divisor
# which is 256K.
def create_uboot_env
    begin
        if $uboot_env_only
            puts "Install U-boot environment config file #{$top}/build/obj/#{$dest}/fw_env.config"
        else
            puts "Install U-Boot environment configuration file (fw_env.config) into #{$mfi_file}"
        end
        sys "echo \"# MTD device name   Device offset   Env. size   Flash sector size\" >> #{$top}/build/obj/#{$dest}/fw_env.config"
        case $mfi_file
        when /lan966x/
            sys "echo \"/dev/mtd1           0x0000         0x40000      0x40000\" >> #{$top}/build/obj/#{$dest}/fw_env.config"
            sys "echo \"/dev/mtd2           0x0000         0x40000      0x40000\" >> #{$top}/build/obj/#{$dest}/fw_env.config"
        when /lan969x/
            sys "echo \"/dev/mtd1           0x0000         0x10000      0x10000\" >> #{$top}/build/obj/#{$dest}/fw_env.config"
            sys "echo \"/dev/mtd2           0x0000         0x10000      0x10000\" >> #{$top}/build/obj/#{$dest}/fw_env.config"
        when /sparx_5/, /ocelot/, /serval_t/, /serval2/
            sys "echo \"/dev/mtd1           0x0000          0x2000      0x40000\" >> #{$top}/build/obj/#{$dest}/fw_env.config"
            sys "echo \"/dev/mtd2           0x0000          0x2000      0x40000\" >> #{$top}/build/obj/#{$dest}/fw_env.config"
        else
            sys "echo \"/dev/mtd2           0x0000          0x2000      0x40000\" >> #{$top}/build/obj/#{$dest}/fw_env.config"
            sys "echo \"/dev/mtd3           0x0000          0x2000      0x40000\" >> #{$top}/build/obj/#{$dest}/fw_env.config"
        end
        if !$uboot_env_only
            sys "install -m 444 #{$top}/build/obj/#{$dest}/fw_env.config #{$dest}/etc"
            sys "rm #{$top}/build/obj/#{$dest}/fw_env.config"
        end

        if $uboot_env_only
            puts "Install U-boot environment config file #{$top}/build/obj/#{$dest}/fw_env_mmc.config"
        else
            puts "Install U-Boot environment configuration file (fw_env_mmc.config) into #{$mfi_file}"
        end
        sys "echo \"# MTD device name   Device offset   Env. size   Flash sector size\" >> #{$top}/build/obj/#{$dest}/fw_env_mmc.config"
        case $mfi_file
        when /lan966x/
            sys "echo \"/dev/mmcblk0p3      0x0000        0x40000      0x40000\" >> #{$top}/build/obj/#{$dest}/fw_env_mmc.config"
            sys "echo \"/dev/mmcblk0p4      0x0000        0x40000      0x40000\" >> #{$top}/build/obj/#{$dest}/fw_env_mmc.config"
        when /lan969x/
            sys "echo \"/dev/mmcblk0p3      0x0000       0x10000        0x40000\" >> #{$top}/build/obj/#{$dest}/fw_env_mmc.config"
            sys "echo \"/dev/mmcblk0p4      0x0000       0x10000        0x40000\" >> #{$top}/build/obj/#{$dest}/fw_env_mmc.config"
        end
        if !$uboot_env_only
            sys "install -m 444 #{$top}/build/obj/#{$dest}/fw_env_mmc.config #{$dest}/etc"
            sys "rm #{$top}/build/obj/#{$dest}/fw_env_mmc.config"
        end
    rescue
        puts "Failed to add Uboot environment configuration file \"fw_env.config\""
    end
end


opt_parser = OptionParser.new do |opts|
    opts.banner = """Usage: tidy [options]

Options:"""
    opts.on("-d", "--destdir path", "Path to destination dir") do |d|
        $dest = d
    end

    opts.on("-t", "--top path", "Dir to root of project") do |d|
        $top = d
    end

    opts.on("-l", "--ld linker", "Linker to use") do |l|
        $ldd = l
    end

    opts.on("-s", "--strip stripper", "Symbol stripper to use") do |s|
        $strip = s
    end

    opts.on("-a", "--arch arch", "CPU Architecture") do |a|
        $arch = a
    end

    opts.on("--api-build path", "API output") do |a|
        $api_path = a
    end

    $uboot_env_only = false
    opts.on("-u", "Create uboot env only") do
        $uboot_env_only = true
    end

    opts.on("-m", "--mfi-name mfi_file", "Name of resulting MFI or ITB file") do |m|
        $mfi_file = m
        case m
        when /^bringup_/
            puts "MFI/ITB File = #{m}"
            $varian = :bringup
        when /^web_/
            puts "MFI/ITB File = #{m}"
            $varian = :web
        when /^smb_/
            puts "MFI/ITB File = #{m}"
            $varian = :smb
        when /^istax_/
            puts "MFI/ITB File = #{m}"
            $varian = :istax
        when /^istax380_/
            puts "MFI/ITB File = #{m}"
            $varian = :istax38x
        when /^istaxmacsec_/
            puts "MFI/ITB File = #{m}"
            $varian = :istaxmacsec
        else
            STDERR.puts("Error: Unable to deduce package type from MFI/ITB filename: #{m}")
            exit(-1)
        end
    end

    opts.on("-h", "--help", "Show this message") do
        puts opts
        exit
    end
end

opt_parser.parse!(ARGV)

if $varian.nil? and !$uboot_env_only
    STDERR.puts("Error: Either -m  or -u option must be specified")
    exit(-1)
end

if $dest.nil?
    STDERR.puts("Error: Destination dir must be specified")
    exit(-1)
end

if $top.nil?
    STDERR.puts("Error: Top dir must be specified")
    exit(-1)
end

# Add the correct version and flavor of FRR.
FRR_VER = "frr-6.0.3-030c175@master"
FRR_LIC = "frr-6.0.3" # Selects the version to make licenses from

if true
    if $uboot_env_only
        # create uboot env config files to use within mmc-rootfs
        create_uboot_env()
        exit(0)
    end

    input_dir = nil
    if File.exist?("#{$top}/vtss_appl/frr/frr_project_src")
        input_dir = "#{$top}/vtss_appl/frr/frr_project_src"
    elsif File.exist?("#{$top}/vtss_appl/frr/daemon/#{FRR_VER}")
        input_dir = "#{$top}/vtss_appl/frr/daemon/#{FRR_VER}"
    else
        sys "sudo mscc-install-pkg -t frr/#{FRR_VER} #{FRR_VER} > /dev/null"
        input_dir = "/opt/mscc/#{FRR_VER}"
    end

    case $mfi_file
    when /serval2/, /serval_t/, /jr2/, /sparxIV/, /lynx2/, /sparx_5/, /lan969/
        zebra_only = false
    else
        # Others don't have L3 H/W support
        zebra_only = true
    end

    zebra_only = true if $varian == :bringup or $varian == :web
    frr_type = zebra_only ? "zebra_only" : "frr"

    # The FRR package contains two different versions of FRR - one meant for
    # bringup and one meant for other packages.
    # The reason that bringup requires another package is that since FRR v7.0,
    # two new libraries (libyang and libpcre) are required to run zebra and
    # the other daemons. These two libraries are roughly 600 Kbytes (compressed)
    # in size, causing the bringup images to become to large, so they will use
    # the last 6.x release, which is v6.0.3
    # The directory layout differs between having only one and multiple versions
    # in the same package. If having multiple, FRR_LIC is defined.
    if defined?(FRR_LIC) and not FRR_LIC.nil?
        # Multiple packages included. Pick the right one.
        frr_ver_to_use = $varian == :bringup ? "frr-6.0.3" : FRR_LIC
        frr_ver_to_use += "/"
    else
        # Only one version included in package.
        frr_ver_to_use = ""
    end

    path = "#{frr_ver_to_use}#{frr_type}"

    puts "Installing #{path} from #{input_dir} into #{$mfi_file}"
    sys "tar -C #{$dest} -xf #{input_dir}/install/#{$arch}/#{path}/root.tar"

    # Remove those daemons not needed in this build.
    sys "rm -f #{$dest}/usr/sbin/ospfd"         unless ENV['MODULES'] =~ /\bfrr_ospf\b/
    sys "rm -f #{$dest}/usr/sbin/ospf6d"        unless ENV['MODULES'] =~ /\bfrr_ospf6\b/
    sys "rm -f #{$dest}/usr/sbin/ripd"          unless ENV['MODULES'] =~ /\bfrr_rip\b/
    sys "rm -f #{$dest}/etc/quagga/ospfd.conf"  unless ENV['MODULES'] =~ /\bfrr_ospf\b/
    sys "rm -f #{$dest}/etc/quagga/ospf6d.conf" unless ENV['MODULES'] =~ /\bfrr_ospf6\b/
    sys "rm -f #{$dest}/etc/quagga/ripd.conf"   unless ENV['MODULES'] =~ /\bfrr_rip\b/

    # .so files are stripped by module_main.in, so we only need to strip the
    # daemons
    strip_flags = "--remove-section=.comment --remove-section=.note"
    sys "#{$strip} #{strip_flags} #{$dest}/usr/sbin/zebra"
    sys "#{$strip} #{strip_flags} #{$dest}/usr/sbin/staticd" if File.exist?("#{$dest}/usr/sbin/staticd")
    sys "#{$strip} #{strip_flags} #{$dest}/usr/sbin/ospfd"   if File.exist?("#{$dest}/usr/sbin/ospfd")
    sys "#{$strip} #{strip_flags} #{$dest}/usr/sbin/ospf6d"  if File.exist?("#{$dest}/usr/sbin/ospf6d")
    sys "#{$strip} #{strip_flags} #{$dest}/usr/sbin/ripd"    if File.exist?("#{$dest}/usr/sbin/ripd")
    create_uboot_env()
end


exit(0)
