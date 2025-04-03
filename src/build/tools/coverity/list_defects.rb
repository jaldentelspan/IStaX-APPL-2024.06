#!/usr/bin/env ruby
#
# Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.
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

require 'pp'
require 'json'
require 'optparse' # For command line parsing

$opt = { }
global = OptionParser.new do |opts|
  opts.banner = "Usage: #{$0} <preview-report-files> ..."
  opts.on("-a", "--all", "List all (Even dismissed)") do
    $opt[:all] = true
  end
  opts.on("-u", "--user <user>", "List only user's issues (regexp)") do |user|
    $opt[:user] = user
  end
end.order!

printf "%-6s %-24s %-14s %-12s %-16s %6s %s\n", "CID", "First Detected", "Owner", "Action", "Classification", 
"Line", "File"
ARGV.each do |f|
  d = JSON.parse(File.read(f))
  cid = d["issueInfo"]
  cid.each do |e|
    t=e["triage"]
    o=e["occurrences"][0]
    if $opt[:all]
      list = true
    else
      list = !(t["classification"].match(/(Intentional|False Positive)/i))
    end
    if $opt[:user] && list
      list = ((t["owner"] || "").match($opt[:user]));
    end
    if list
      printf "%-6s %-24s %-14s %-12s %-16s %6d %s\n", e["cid"], e["firstDetectedDateTime"], 
      t["owner"], t["action"], t["classification"], o["mainEventLineNumber"],  File.basename(o["file"])
    end
  end
end
