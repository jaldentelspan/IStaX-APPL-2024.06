#!/usr/bin/env ruby
#
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
#

require 'optparse'  # For command line parsing
require 'pp'        # For pretty-printing objects

# Parsing input from command line
$options = {}
OptionParser.new do |opts|
    opts.banner = "Usage: fixdeps [options] from-string to-string infile ..."

    opts.on("-o", "--out <out>",  "Set output file") do |out|
        $options[:out] = out
    end

    opts.on("-h", "--help", "Show this message") do
        puts opts
        exit
    end
end.parse!

def process(infile, outfd, from, to)
    File.open(infile).each do |line|
        outfd.puts line.gsub(/^#{Regexp.quote(from)}:/, "#{to}:")
    end
end

if $options[:out]
      outfile = File.new($options[:out], "w")  or  die("Cannot create output file #{$options[:out]}")
else
      outfile = $stdout
end

from = ARGV.shift
to   = ARGV.shift

ARGV.each { |infile|
    process(infile, outfile, from, to)
}
