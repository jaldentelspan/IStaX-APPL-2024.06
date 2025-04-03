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

require 'pp'
require 'pty'
require 'json'
require 'open3'
require 'thread'
require 'optparse'
require 'pathname'

$opt_path = nil
$opt_threads = (%x{grep -c ^processor /proc/cpuinfo}.to_i * 1.5).to_i

$opt_color = 0

if $stdout.isatty
    $opt_color = 1
end

$cnt_ok = 0
$cnt_err = 0
work_q = Queue.new
mutex = Mutex.new

d = %x{mktemp -d}.chomp
dd = "#{d}/vtss"
inc_microchip = "#{d}/microchip"

puts "Running: #{d}"

%x{mkdir #{dd}}
%x{mkdir #{inc_microchip}}

%x{cp -r $API_BUILD_PATH/include_common/microchip #{d}/.}
%x{cp -r $API_BUILD_PATH/include_common/*.h #{d}/.}
%x{cp -r ../../vtss_appl/include/vtss/appl #{dd}/.}
%x{cp -r ../../vtss_basics/include/vtss/basics #{dd}/.}
%x{cp -r ../../vtss_basics/platform/linux/include/vtss/basics/* #{dd}/basics/.}

Dir.chdir d

Dir.glob("**/*.{h,hxx}").sort().each do |e|
    next if e == "microchip/ethernet/hdr_end.h"
    next if e == "microchip/ethernet/hdr_start.h"
    next if e == "microchip/ethernet/board/api/hdr_end.h"
    next if e == "microchip/ethernet/board/api/hdr_start.h"
    next if e == "vtss/appl/optional_modules_create.hxx"

    cmd = "g++ -std=c++17 -DVTSS_BASICS_STANDALONE -I. #{e} ; rm -f #{e}.gch"
    work_q.push cmd
end

workers = (0...$opt_threads).map do
    Thread.new do
        begin
            while cmd = work_q.pop(true)
                o = ""
                e = ""
                s = 0

                if $opt_color == 1
                    PTY.spawn(cmd) do |r, w, pid|
                        begin
                            while !r.eof?
                                e += r.readpartial(4096)
                            end
                        rescue Errno::EIO
                        end

                        Process.wait(pid)
                    end
                    s = $?.to_i
                else
                    o, e, s = Open3.capture3(cmd)
                end

                mutex.synchronize {
                    if s != 0
                        STDERR.puts ""
                        STDERR.puts "HEADER-CHECK-ERROR: #{cmd}"
                        STDERR.puts e
                        STDERR.puts ""
                        $cnt_err += 1
                    else
                        $cnt_ok += 1
                        puts "OK: #{cmd}"
                    end
                }

            end
        rescue ThreadError
        end
    end
end

workers.map(&:join)

%x{rm -rf #{d}}

if $cnt_err > 0
    STDERR.puts "Total: #{$cnt_ok + $cnt_err}, OK: #{$cnt_ok}, Error: #{$cnt_err}"
    exit 1
else
    puts "Total: #{$cnt_ok + $cnt_err}, OK: #{$cnt_ok}, Error: #{$cnt_err}"
    exit 0
end

