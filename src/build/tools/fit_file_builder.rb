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

require 'slop'
require 'pathname'
require 'tempfile'

#################################################################
# Shell helper
#################################################################

def sys(cmd)
    return %x(#{cmd})
end

def syscmd(cmd)
    puts "Running #{cmd}" if $options[:verbose]
    response = %x(#{cmd})
    if $? != 0
        raise "Running '#{cmd}' failed"
    end
    return response
end

module Slop
    class PathOption < Option
        def call(value)
            Pathname.new(value).to_s
        end
    end
end

def arguments
    opts = Slop::Options.new
    opts.banner =  "Usage: fit_file_builder.rb parse [options]"
    opts.separator ""
    opts.separator "Most used options:"
    opts.string    "-a", "--arch", "Target build architecture", default: "arm64"
    opts.array     "-b", "--board", "Target board(s) (list of DTBs)", required: true, delimiter: ':'
    opts.string    "-k", "--kernel-file", "Path to kernel image", required: true
    opts.string    "-s", "--priv_key_path", "Path to hash keys within kernel"
    opts.string    "-m", "--machine", "Target machine", required: true
    opts.string    "-o", "--output", "Output file", required: true
    opts.string    "-r", "--dtb-path", "Path to Device tree files", required: true
    opts.string    "-O", "--dtb-overlay", "Device tree overlay file (optional)"
    opts.string    "-V", "--version-string", "Software version string (optional)"
    opts.string    "-D", "--date-string", "Software build date (optional)"
    opts.string    "-R", "--rev-string", "Software revision (optional)"
    opts.separator ""
    opts.separator "Extra options:"
    opts.string    "--rootfs", "Path to rootfs file (squashfs or similar)"
    opts.string    "--uboot-path", "Path to U-Boot binary"
    opts.bool      "-d", "--debug", "Show values of options"
    opts.bool      "-v", "--verbose", "Enable verbose mode"
    opts.on        "-h", "--help", "Show this message" do
        puts opts
        exit
    end
    opts
end

def parse_arguments(command_line_options=ARGV, parser)
    begin
        result = parser.parse command_line_options
    rescue Slop::Error => e
        # print help
        puts "Rescued: #{e.inspect}"
        puts arguments
        exit
    end
end

# Create an argument parser
parser = Slop::Parser.new(arguments)
# Parse arguments and store them into the $options hash
$options = parse_arguments(ARGV, parser).to_hash
# Populate the $options hash with additional
# key, value pairs that are not directly derived
# from the script's arguments.
$options[:kcomp] = "gzip"
$options[:kernel_name] = "mscc-linux-#{$options[:machine]}.gz"
$options[:arch] = "mips" if $options[:arch] == "mipsel"

if $options[:arch] != "arm64"
    # Currently, we only support secure boot on ARM64
    $options.delete(:priv_key_path)
end

if $options[:debug]
    $options.each do |key, value|
        puts " -- #{key} = #{value}"
    end
end

def create_its_file(kernel)
    $name = "#{$options[:workarea]}/fitimage.its"
    $src_file = File.new($name, "w")
    #...
    $src_file.puts("/dts-v1/;")
    $src_file.puts("/ {")
    $src_file.printf("        description = \"FIT image file for Webstax on %s", $options[:machine])
    if $options[:version_string]
      $src_file.printf("\n                       @(#)Version: %s", $options[:version_string])
    end
    if $options[:date_string]
      $src_file.printf("\n                       @(#)Date   : %s", $options[:date_string])
    end
    if $options[:rev_string]
      $src_file.printf("\n                       @(#)Rev    : %s", $options[:rev_string])
    end
    $src_file.printf("\";\n")
    $src_file.puts("        images {")
    $src_file.puts("                kernel {")
    $src_file.puts("                        description = \"Linux kernel\";")
    $src_file.puts("                        data = /incbin/(\"#{kernel}\");")
    $src_file.puts("                        type = \"kernel\";")
    $src_file.puts("                        arch = \"#{$options[:arch]}\";")
    $src_file.puts("                        os = \"linux\";")
    $src_file.puts("                        compression = \"#{$options[:kcomp]}\";")
    case $options[:arch]
    when "mips"
        $src_file.puts("                        load = <0x80100000>;")
        $src_file.puts("                        entry = <0x80100000>;")
    when "arm64"
        if $options[:machine] == "ls1046"
            $src_file.puts("                        load = <0x80080000>;")
            $src_file.puts("                        entry = <0x80080000>;")
        elsif $options[:machine] == "lan969x"
            $src_file.puts("                        load = /bits/ 64 <0x60000000>;")
            $src_file.puts("                        entry = /bits/ 64 <0x60000000>;")
        else
            $src_file.puts("                        load = /bits/ 64 <0x700080000>;")
            $src_file.puts("                        entry = /bits/ 64 <0x700080000>;")
        end
    when "arm"
        if $options[:machine] == "bbb"
            $src_file.puts("                        load = <0x80080000>;")
            $src_file.puts("                        entry = <0x80080000>;")
        else
            $src_file.puts("                        load = <0x60208000>;")
            $src_file.puts("                        entry = <0x60208000>;")
        end
    else
      raise "Unsupported architecture: #{$options[:arch]}"
    end
    if $options[:priv_key_path]
        $src_file.puts("                        hash-1 {")
        $src_file.puts("                                algo = \"sha1\";")
        $src_file.puts("                        };")
    end
    $src_file.puts("                };")
    if $options[:rootfs]
        rootpath = File.realpath($options[:rootfs])
        $src_file.puts("                ramdisk {")
        $src_file.puts("                        description = \"ramdisk\";")
        $src_file.puts("                        data = /incbin/(\"#{rootpath}\");")
        $src_file.puts("                        type = \"ramdisk\";")
        $src_file.puts("                        arch = \"#{$options[:arch]}\";")
        $src_file.puts("                        os = \"linux\";")
        $src_file.puts("                        compression = \"none\";")

        if $options[:machine] == "lan966x"
            $src_file.puts("                        load = <0x68000000>;")
        elsif $options[:machine] == "bbb"
            $src_file.puts("                        load = <0x88080000>;")
        end
        if $options[:machine] == "lan969x"
            $src_file.puts("                        load = /bits/ 64 <0x61000000>;")
        end
        if $options[:priv_key_path]
            $src_file.puts("                        hash-1 {")
            $src_file.puts("                                algo = \"sha1\";")
            $src_file.puts("                        };")
        end
        $src_file.puts("                };")
    end
    $options[:board].each do |pcb|
        fdt_in  = "#{$options[:dtb_path]}/#{pcb}.dtb"
        fdt_out = "#{$options[:workarea]}/#{pcb}.dtb"
        if $options[:dtb_overlay]
            syscmd "fdtoverlay -i #{fdt_in} -o #{fdt_out} #{$options[:dtb_overlay]}"
        else
            FileUtils.cp(fdt_in, fdt_out)
        end
        $src_file.puts("                fdt_#{pcb} {")
        $src_file.puts("                        description = \"Flattened Device Tree blob\";")
        $src_file.puts("                        data = /incbin/(\"#{fdt_out}\");")
        $src_file.puts("                        type = \"flat_dt\";")
        $src_file.puts("                        arch = \"#{$options[:arch]}\";")
        $src_file.puts("                        compression = \"none\";")
        if $options[:machine] == "ls1046"
            $src_file.puts("                        load = <0x90000000>;")
        elsif $options[:machine] == "lan966x"
            $src_file.puts("                        load = <0x67e00000>;")
        elsif $options[:machine] == "lan969x"
            $src_file.puts("                        load = /bits/ 64 <0x6fff0000>;")
        end
        if $options[:priv_key_path]
            $src_file.puts("                        hash-1 {")
            $src_file.puts("                                algo = \"sha1\";")
            $src_file.puts("                        };")
        end
        $src_file.puts("                };")
    end
    $src_file.puts("        };")
    $src_file.puts("")
    $src_file.puts("        configurations {")
    $options[:board].each_with_index do |pcb, ix|
        label = pcb.sub(/\w+_pcb/, "pcb")
        if $options[:machine] == "lan966x"
          tmp = pcb.sub(/\w+-\w+-pcb/, "")
          if tmp == "8290" || tmp =="8385"
            label = "lan9668_ung" + tmp + "_0_at_lan966x"
          else
            label = "lan9662_ung" + tmp + "_0_at_lan966x"
          end
        elsif $options[:machine] == "bbb"
            label = "6849_1@bbb"
        end
        puts "board = #{$options[:board]}, pcb = #{pcb}\n"
        if $options[:machine] == "lan969x"
          pcb_id = pcb.sub(/\w+_appl_/, "")
          if pcb_id == "ev23x71a"
            label = "lan9698_" + pcb_id + "_0_at_lan969x"
          end
        end
        puts "label = #{label}\n"
        if $options[:priv_key_path]
          puts "Using private keys @ #{$options[:priv_key_path]}\n"
        end

        $src_file.printf("		default = \"%s\";\n", label) if ix == 0
        $src_file.puts("                #{label} {")
        $src_file.puts("                        description = \"Boot Linux kernel with DT for #{pcb}\";")
        $src_file.puts("                        kernel = \"kernel\";")
        $src_file.puts("                        fdt = \"fdt_#{pcb}\";")
        if $options[:rootfs]
            $src_file.puts("                        ramdisk = \"ramdisk\";")
        end
        if $options[:priv_key_path]
            $src_file.puts("                        signature-1 {")
            $src_file.puts("                                algo = \"sha1,rsa2048\";")
            $src_file.puts("                                key-name-hint = \"dev\";")
            $src_file.puts("                                sign-images = \"fdt\", \"kernel\", \"ramdisk\";")
            $src_file.puts("                        };")
            $src_file.puts("                        hash-1 {")
            $src_file.puts("                                algo = \"sha1\";")
            $src_file.puts("                        };")
        end
        $src_file.puts("                };")
    end
    $src_file.puts("        };")
    $src_file.puts("};")
    $src_file.puts("")
    #...
    $src_file.close
end

def get_kernel_image()
  kfile = $options[:workarea] + "/" + $options[:kernel_name]
  if $options[:kernel_file].match(/\.gz$/)
    kfile = $options[:kernel_file]
  elsif $options[:kernel_file].match(/\.xz$/)
    syscmd("xz -d -c #{$options[:kernel_file]} | gzip -f -c > #{kfile}")
  else
    syscmd("gzip -f -c > #{kfile} < #{$options[:kernel_file]}")
  end
  return kfile
end

def make_fit_file()
    if $options[:priv_key_path]
      syscmd("mkimage -k #{$options[:priv_key_path]} -f #{$options[:workarea]}/fitimage.its #{$options[:output]}")
    else
      syscmd("mkimage -f #{$options[:workarea]}/fitimage.its #{$options[:output]}")
    end
end

def create_fit_file()
    Dir.mktmpdir('fitbuild') do |dir|
        $options[:workarea] = dir
        kfile = get_kernel_image()
        create_its_file(kfile)
        make_fit_file()
    end
end

create_fit_file()
