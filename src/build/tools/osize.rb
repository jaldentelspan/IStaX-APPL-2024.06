#!/usr/bin/env ruby
#
# Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.
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

require 'optparse'  # For command line parsing
require 'logger'    # For trace
require 'pp'        # For pretty-printing objects

$script_name_with_path = File.expand_path($0)
$script_name           = File.basename($script_name_with_path)
$top_path              = `git rev-parse --show-toplevel`.strip!

# Parsing input from command line
$options = {}
OptionParser.new do |opts|
    opts.banner = "Usage: #{$script_name} [options] [elf-and/or-o-file(s)]\n
                  If no input files are given, the script attempts to look into the obj folder and uses all the .o  and .elf files (recursively)."

    $options[:verbosity] = :err
    opts.on("-v", "--verbosity  <lvl>", [:info, :debug], "Set verbosity level.") do |lvl|
        $options[:verbosity] = lvl
    end
end.parse!

$l = Logger.new(STDOUT)
$l.level = Logger::ERROR
$l.level = Logger::INFO  if $options[:verbosity] == :info
$l.level = Logger::DEBUG if $options[:verbosity] == :debug

files = ARGV
if files.size() != 0
    $l.info("Got #{files} on command line")
    files = Dir.glob(files)
else
    obj = "#{$top_path}/build/obj"
    $l.info("Looking for .o and .elf files recursively in #{obj}/")
    files = Dir[File.join("#{obj}/", '**', '*')].reject {|p| File.directory?(p) || (!p.match(/\.o$/) && !p.match(/\.elf$/))}

    #$l.info("Looking for .o and .elf files recursively in CWD")
    #files = Dir[File.join("./", '**', '*')].reject {|p| File.directory?(p) || (!p.match(/\.o$/) && !p.match(/\.elf$/))}
end

if files.size() == 0
    STDERR.puts("No input files given and no .elf file(s) found in obj folder")
    exit(-1)
end

def do_cmd(cmd)
    res = `#{cmd}`
    if !$?.success?
        STDERR.puts("\"#{cmd}\" failed.")
        exit(-1)
    end

    return res
end

$res = Array.new()

files.each {|file|
    $l.info("Considering #{file}")
    r = do_cmd("file #{file}")
    if r.match(/x86/)
        STDERR.puts("Skipping #{file}, because it's for the local host, not the target")
        next
    end

    r = do_cmd("size #{file}").split(/\n/).join()
    if match = r.match(/(\d+)\s*(\d+)\s*(\d+)/)
       res = Hash.new()
       res[:path] = file
       res[:file] = File.basename(file)
       res[:text] = match[1].to_i()
       res[:data] = match[2].to_i()
       res[:bss]  = match[3].to_i()
       $res.push(res)
    else
        STDERR.puts("Unable to find txt, data, and bss, of #{file}")
    end
}

# Pretty print
def my_print()
    wid = Hash.new()
    wid[:file] = 65
    wid[:text] = 10
    wid[:data] = 10
    wid[:bss]  = 10

    print_what = [:file, :text, :data, :bss]

    str1 = ''
    str2 = ''
    print_what.each {|what|
        str1 += ' ' if str1.length() != 0
        str1 += sprintf("%-*s", wid[what], what)
        str2 += ' ' if str2.length() != 0
        str2 += sprintf("%s", '-' * wid[what])
    }

    puts(str1)
    puts(str2)

    sum = Hash.new()
    [".o", ".elf"].each {|type|
        sum[type] = Hash.new()
        sum[type][:text] = 0
        sum[type][:data] = 0
        sum[type][:bss]  = 0
    }

    $res.each {|k|
        str = ''
        print_what.each {|what|
            str += ' ' if str.length() != 0
            str += sprintf("%-*s", wid[what], k[what]) if what == :file
            str += sprintf("%*d",  wid[what], k[what]) if what != :file
        }

        puts(str)

        [:text, :data, :bss].each {|kind|
            ext = File.extname(k[:file])
            sum[ext][kind] += k[kind]
        }
    }

    str = ''
    print_what.each {|what|
        str += ' ' if str.length() != 0
        str += sprintf("%s", '-' * wid[what])
    }

    puts(str)

    sum.each {|k, v|
        str = ''
        print_what.each {|what|
            str += ' ' if str.length() != 0
            str += sprintf("%-*s", wid[what], "Sum of #{k}") if what == :file
            str += sprintf("%*d",  wid[what], sum[k][what])  if what != :file
        }

        puts(str)
    }

    puts("")
end

# Sort by a key
def sort_by(key, ascending = 1)
    $res.sort! {|b, c|
        ascending != 0 ? b[key] <=> c[key] : c[key] <=> b[key]
    }

    puts(sprintf("Sorted %sscending by #{key}", ascending != 0 ? 'a' : 'de'))
    my_print()
end

sort_by(:file)
sort_by(:text, 0)
sort_by(:data, 0)
sort_by(:bss, 0)

