#!/usr/bin/env ruby
#
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
#

require 'pp'
require 'logger'
require 'optparse' # For command line parsing
require 'fileutils'

$l = Logger.new(STDERR)
$l.level = Logger::WARN

$opt = {:limit => 12}

cfg = File.readlink("config.mk")
if !File.exist?(cfg)
  $l.fatal "Config #{cfg} does not exist"
  exit 1
end
$opt[:stream] = File.basename(cfg, ".mk")
$opt[:dir]    = "obj/cov/cov-#{$opt[:stream]}"
$opt[:strip]  = Dir.pwd.sub(/\/build$/, "/");

global = OptionParser.new do |opts|

  opts.banner = "Usage: #{$0} "

  opts.on("-n", "--dry-run", "Don't do - just show") do
    $opt[:dryrun] = 1
    $l.level = Logger::INFO
  end

  opts.on("-s", "--stream <stream>", "Use this stream (default #{$opt[:stream]})") do |stream|
    $opt[:stream] = stream
  end

  opts.on("-d", "--dir <dir>", "Use this directory for Coverity results (default #{$opt[:dir]})") do |dir|
    $opt[:dir] = dir
  end

  opts.on("-l", "--limit <lim>", "Limit process count (parallel make)") do |lim|
    $opt[:limit] = lim.to_i
  end

  opts.on("-C", "--no-clean", "Don't clean before building (do incremental build)") do
    $opt[:noclean] = 1
  end

  opts.on("-B", "--no-build", "Don't do build (or clean)") do
    $opt[:nobuild] = 1
  end

  opts.on("-A", "--no-analyze", "Don't do cov-analyze (or build). Implies --preview-report") do
    $opt[:noanalyze] = 1
    $opt[:nobuild] = 1
    $opt[:preview] = 1
  end

  opts.on("-p", "--preview-report", "Produce a preview report") do
    $opt[:preview] = 1
  end

  opts.on("-D", "--debug", "Set debug (see command output)") do
    $l.level = Logger::DEBUG
  end

  opts.on("-v", "--verbose", "Set verbose") do
    $l.level = Logger::INFO
  end

end.order!

def run_cmd(cmd)
  if !$opt[:dryrun]
    $l.info "Running '#{cmd}' ..."
    o=%x(#{cmd})
    $l.debug o
    if $? != 0
      $l.fatal "Running '#{cmd}' failed (rc #{$?})"
      exit $?
    end
    return o
  else
    $l.info "NOT running '#{cmd}' ..."
    return ""
  end
end

# Loose CCACHE
ENV["BR_NO_CCACHE"]="please"
ENV["CCACHE"]=nil

# Path to Coverity suite
ENV["PATH"] += ":/opt/coverity/cov-analysis-linux64-2023.9.2/bin"

if !$opt[:nobuild]
  if !$opt[:noclean]
    $l.info "Cleaning up everything first"
    run_cmd("rm -rf obj")
  else
    $l.info "Only cleaning #{$opt[:dir]} folder"
    run_cmd("rm -rf obj/cov/")
  end
  run_cmd("mkdir -p #{$opt[:dir]}")
  $l.info "Building #{cfg} in #{$opt[:dir]}"
  run_cmd("make coverity_cfg")
  run_cmd("cov-build --return-emit-failures --parse-error-threshold 50 --config obj/coverity.xml --dir #{$opt[:dir]} make -j #{$opt[:limit]}")
end

if !$opt[:noanalyze]
  dir=File.dirname(__FILE__)
  tpopts = File.read("#{dir}/exclude.txt").tr_s("\n", " ")
  scanopts = File.read("#{dir}/scanopts.txt").tr_s("\n", " ")
  $l.info "Analyzing #{cfg} in #{$opt[:dir]}"
  run_cmd("cov-analyze -V 3 -tp '#{tpopts}' --dir #{$opt[:dir]} --strip-path #{$opt[:strip]} #{scanopts}")
end

if $opt[:preview]
  json = "#{$opt[:dir]}/result.json"
  html = "#{$opt[:dir]}/html/"
  $l.info "Generating preview report for #{$opt[:stream]}"

  run_cmd("cov-format-errors --dir #{$opt[:dir]} --json-output-v6 #{json}")
  run_cmd("cov-format-errors --dir #{$opt[:dir]} --html-output #{html}")

  puts("JSON results in #{json}")
  puts("HTML results in #{html}")

  # This script used to commit the defects to the Coverity server by using the
  # following command. The ./run.rb script does exactly this and is invoked on a
  # daily basis by a Jenkins job.
#  run_cmd("cov-commit-defects --dir #{$opt[:dir]} --preview-report-v2 report-#{$opt[:stream]}.json --stream #{$opt[:stream]} --config /opt/coverity/credentials.xml --auth-key-file /opt/coverity/reporter.key")
end
