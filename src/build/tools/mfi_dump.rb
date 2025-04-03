#!/usr/bin/env ruby

# Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.
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

require 'find'
require 'pp'
require 'open3'

top_path = `git rev-parse --show-toplevel`.strip!

def run(cmd, additional = nil)
    stdout, stderr, status = Open3.capture3(cmd)
    if !status.success?
        STDERR.puts("ERROR: When running \"#{cmd}\": #{stderr}")
    end

    return stdout
end

mfi_files = []
Find.find("#{top_path}/build/obj/") do |path|
  mfi_files << path if path =~ /.*\.mfi$/
end

=begin
Stage1
  Version:2
  Magic1:0xedd4d5de, Magic2:0x987b4c4d, HdrLen:92, ImgLen:2244324
  Machine:jaguar2c, SocName:jaguar2, SocNo:7, SigType:1
  Tlv Type:Kernel(0), Data Length:2028760
  Tlv Signature(1),   Data Length:16 (validated)
  Tlv Initrd(2),      Data Length:196608
  Tlv KernelCmd(3),   Data Length:48
  Tlv Metadata(4),    Data Length:73
  Tlv License(5),     Data Length:18632
Stage2 - Index:0
  Tlv FsElement(2), Data Length:5388070
  MD5(1), Length:16 Data: 00074fd9446545c09a1f003b50efe377 (validated)
    Name:                           rootfs
    Version:                        2020.02.3-314
    License:                        Found
    Content file name:              /opt/mscc/mscc-brsdk-mipsel-2020.02.3-314/mipsel-mips32r2-linux-gnu/smb/rootfs.squashfs
    Content (squashfs) file length: 5296128
Stage2 - Index:1
  Tlv FsElement(2), Data Length:4591683
  MD5(1), Length:16 Data: 103e1dbe027f5bb3854a8fc3d5c8b94d (validated)
    Name:                           vtss
    Version:                        88b3f74b43
    Content file name:              web_jr2_24.app-rootfs
    Content (squashfs) file length: 4591616
Stage2 - Index:2
  Tlv FsElement(2), Data Length:446541
  MD5(1), Length:16 Data: e2e7f865e22dbf0ac527a88c416c585b (validated)
    Name:                           vtss-web-ui
    Version:                        88b3f74b43
    Content file name:              vtss-www-rootfs.squashfs
    Content (squashfs) file length: 446464
=end

largest_stage1_size = 0
largest_stage2_size = 0
largest_stage1_file = ""
largest_stage2_file = ""

largest_file_size   = 0
largest_file        = ""

mfi_files.each {|file|
   puts("#{file}:")

   in_stage1   = false
   in_stage2   = false
   stage1_size = 0
   stage2_size = 0
   file_size   = File.size(file)

   if (file_size > largest_file_size)
       largest_file_size = file_size
       largest_file      = file
   end

   run("mfi.rb -d -i #{file}").each_line {|line|
       case line
       when /^Stage1/
           in_stage1 = true
           in_stage2 = false

       when /^Stage2/
           in_stage1 = false
           in_stage2 = true

       when /ImgLen:(\d+)/
           STDERR.puts("ERROR: Not in stage1") if not in_stage1
           stage1_size += $1.to_i()

       when /Data Length:(\d+)/
           stage2_size += $1.to_i() if in_stage2
       end
   }

   if (stage1_size > largest_stage1_size)
       largest_stage1_size = stage1_size
       largest_stage1_file = file
   end


   if (stage2_size > largest_stage2_size)
       largest_stage2_size = stage2_size
       largest_stage2_file = file
   end

   puts("  Stage1 = #{stage1_size} bytes")
   puts("  Stage2 = #{stage2_size} bytes")
   puts("  Total  = #{stage1_size + stage2_size} bytes")
   puts("  File   = #{file_size} bytes")
   puts()
}

puts("Largest Stage1 = #{largest_stage1_size} bytes = #{largest_stage1_size / 1024} KBytes (#{largest_stage1_file})")
puts("Largest Stage2 = #{largest_stage2_size} bytes = #{largest_stage2_size / 1024} KBytes (#{largest_stage2_file})")
puts("Largest File   = #{largest_file_size} bytes = #{largest_file_size / 1024} KBytes (#{largest_file})")
puts()

