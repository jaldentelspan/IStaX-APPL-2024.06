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
    $l.info("Looking for .o  files recursively in #{obj}/")
    files = Dir[File.join("#{obj}/", '**', '*')].reject {|p| File.directory?(p) || (!p.match(/\.o$/))}
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
       res[:file] = File.basename(file, ".o")
       res[:text] = match[1].to_i()
       res[:data] = match[2].to_i()
       res[:bss]  = match[3].to_i()
       $res.push(res)
    else
        STDERR.puts("Unable to find txt, data, and bss, of #{file}")
    end
}

# Pretty print
$grps = %w{ip_source}

$specials = {
  'mstp' => [ %r{^(Topology|Bridge|Port)}  ],
  'rfcmibs' => [ %r{^rfc[0-9]{4}}  ],
  '802.1x' => [ %r{^ieee8021} ],
  'zls' => [ %r{^zls?} ],
  'lldpmed' => [ %r{^(lldpmed|lldpXMed)} ],
}

def sum_print(data)
    wid = Hash.new()
    wid[:module] = 28
    wid[:text] = 10
    wid[:data] = 10
    wid[:bss]  = 10
    wid[:files]  = 5

    print_what = [:module, :text, :data, :bss, :files]

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

    data.each {|g,k|
      str = ''
      print_what.each {|what|
        str += ' ' if str.length() != 0
        if what == :module
          if k[:files] == 1
            str += sprintf("%-*s", wid[:module], k[:file])
          else
            str += sprintf("%-*s", wid[:module], g)
          end
        else
          str += sprintf("%*d",  wid[what], k[what])
        end
      }

      puts(str)

    }

end

def get_grp(fn)
  $specials.each{|g,r|
    r.each{|m|
      if fn =~ m
        return g
      end
    }
  }
  $grps.each {|g|
    if fn == g or fn =~ /^#{g}_/
      return g
    end
  }
  e = fn.split(/[_-]/)
  if e[0] == "vtss"
    return e[1]
  end
  if e[0] != ""
    return e[0]
  end
  return nil
end

$grpsum = {}
$res.each {|k|
  g = get_grp(k[:file])
  if g != nil
    if $grpsum[g] != nil
      $grpsum[g][:text] += k[:text]
      $grpsum[g][:data] += k[:data]
      $grpsum[g][:bss]  += k[:bss]
      $grpsum[g][:files] += 1;
    else
      $grpsum[g] = { :file => k[:file],
                     :text => k[:text],
                     :data => k[:data],
                     :bss => k[:bss],
                     :files => 1,
                   }
    end
  end
}

#pp $grpsum

gt = {
  :text  => 0,
  :bss   => 0,
  :data  => 0,
  :files => 0,
}
$grpsum.each { |k,v|
  gt[:text]  += v[:text]
  gt[:data]  += v[:data]
  gt[:bss]   += v[:bss]
  gt[:files] += v[:files]
}

$grpsum['totals'] = gt
$grpsum = $grpsum.sort_by {|k,v|
  -v[:text]
}

sum_print($grpsum)
