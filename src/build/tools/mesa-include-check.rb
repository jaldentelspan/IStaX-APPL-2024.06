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

$api_include = []

# DO NEVER NEVER ADD ANY NEW FILES TO THIS LIST - ONLY REMOVING FILES IS OKAY!!!
$exceptions = [
]

def included_in_exception f
    $exceptions.each do |e|
        if File.fnmatch(e, f)
            return true
        end
    end
    return false
end

opt_parser = OptionParser.new do |opts|
    opts.banner = """Usage: mesa-include-check.rb [options]

Options:"""
    opts.on("-a", "--api-include path", "API Include path (may be repeated)") do |p|
        $api_include << p
    end

    opts.on("-p", "--path path", "Path to check") do |p|
        $opt_path = p
    end

    opts.on("-h", "--help", "Show this message") do
        puts opts
        exit
    end
end

opt_parser.parse!(ARGV)

api_hdrs = []
$api_include.each do |a|
    Dir.glob(File.join("#{a}/**", "*.{h,hxx,hh,hpp}")) do |h|
        file = File.basename h
        case h
        when /microchip\/ethernet\//
            # skip it
        when /vtss\/api\//
            api_hdrs << "vtss/api/#{file}"
        else
            api_hdrs << file
        end
    end
end

if api_hdrs.size == 0
    $stderr.puts "No API headers found!"
    exit 1
end

puts "Checking where the Unified API headers are included ([#{api_hdrs.join(", ")}])"

Dir.chdir $opt_path if $opt_path

err = 0
types = ["*.c", "*.cc", "*.cxx", "*.h", "*.hh", "*.hpp", "*.hxx", "*.icli"]
c = %Q{| find . \\( #{types.collect{|x| "-name \"#{x}\""}.join(" -or ")} \\) -exec grep -H "\\#include" {} \\;}

open(c, "r").each_line do |l|
    file, tail = l.split ":", 2

    hdr = nil
    if /^\s*#\s*include\s*<(.*)>/ =~ tail
        hdr = $1
    elsif /^\s*#\s*include\s*"(.*)"/ =~ tail
        hdr = $1
    else
        #puts "#{file}  -> #{tail}"
    end

    if hdr and api_hdrs.include? hdr
        ext = File.extname file

        case ext
        when ".h", ".hxx", ".hpp", ".hh"
            $stderr.puts "#{hdr} is included in #{file}. It is not allowed to include the Unified-API headers in any application header!"
            err += 1
        when ".c", ".cxx", ".icli", ".cc"
            if not included_in_exception file
                $stderr.puts "#{hdr} is included in #{file}. It is not allowed to re-introduce the Unified-API headers in compilation units which has been cleaned up!"
                err += 1
            end
        end
    end
end

exit err

