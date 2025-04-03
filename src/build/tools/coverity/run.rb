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
require 'csv'

$covopts = "--config /opt/coverity/credentials.xml --auth-key-file /opt/coverity/reporter.key"

$l = Logger.new(STDERR)
$l.level = Logger::WARN

@jobs = Hash.new

$opt = { :project => "webstax" }
global = OptionParser.new do |opts|

  opts.banner = "Usage: #{$0} "

  opts.on("-P", "--project <project>", "Use this Coverity project") do |project|
    $opt[:project] = project
  end

  opts.on("-p", "--preview-report", "Don't commit defects, but produce preview report") do
    $opt[:preview] = 1
  end

  opts.on("-d", "--description <desc>", "Set description for committing results") do |desc|
    $opt[:description] = desc
  end

  opts.on("-V", "--version <vers>", "Set version for committing results") do |vers|
    $opt[:version] = vers
  end

  opts.on("-D", "--debug", "Set debug (see command output)") do
    $l.level = Logger::DEBUG
  end

  opts.on("-v", "--verbose", "Set verbose") do
    $l.level = Logger::INFO
  end

end.order!

def run_cmd(cmd, exit_on_err = true)
  if !$opt[:dryrun]
    $l.info "Running '#{cmd}' ..."
    o=%x(#{cmd})
    $l.debug o
    if $? != 0
      $l.fatal "Running '#{cmd}' failed"
      exit 1 if exit_on_err == true
    end
    return o
  else
    $l.info "NOT running '#{cmd}' ..."
    return ""
  end
end

def coverity (cfg, f)
  $l.info "Building #{cfg}"
  covdir = "obj/cov/cov-#{cfg}"
  cmd = "./tools/coverity/build_coverity.sh #{f} #{covdir}"
  run_cmd(cmd)
end

def commit (cfg)
  covdir = "obj/cov/cov-#{cfg}"
  if Dir.exists?(covdir)
    $l.info "Committing #{cfg} from #{covdir}"
    cmd = "cov-commit-defects"
    cmd << " #{$covopts}"
    cmd << " --stream #{cfg}"
    cmd << " --dir #{covdir}"
    if $opt[:preview]
      cmd << " --preview-report-v2 #{cfg}.json"
    else
      cmd << %Q{ --description "#{$opt[:description]}"} if $opt[:description]
      cmd << %Q{ --version "#{$opt[:version]}"} if $opt[:version]
    end
    run_cmd(cmd, false)
    FileUtils.rm_rf(covdir)
  else
    $l.warn "Unable to find #{covdir}"
  end
end

# args = files
files = ARGV

# base dir
pwd = Dir.pwd

# base dir
basedir = File.expand_path(File.dirname(__FILE__))

def get_streams(project)
  streams = []
  IO.popen("cov-manage-im --mode streams --show #{$covopts} --project #{project}").each do |line|
    data = line.chop.parse_csv
    if data[0] != "Stream"
      stream = data[0]
      $l.info "Have stream #{stream}"
      streams <<= stream
    end
  end
  return streams
end

def add_streams(project, streams)
    streams.each do |stream|
        $l.warn("Add %s to %s" % [stream, project])
        run_cmd("cov-manage-im #{$covopts} --mode streams --add --set name:#{stream} --set lang:cpp --set component-map:#{project} --set ownerAssignmentOption:scm --set expiration:enabled --set triage:#{project} --set desc:\"Configuration #{stream}\"")
        run_cmd("cov-manage-im #{$covopts} --mode projects --update --name #{project} --insert stream:#{stream}")
    end
end

streams = get_streams($opt[:project])
need_streams = files.map { |cfg| File.basename(cfg, ".mk") }

diff = need_streams - streams
add_streams($opt[:project], diff)

files.each do |f|
  cfg = File.basename(f, ".mk")
  $l.debug "Stream '#{cfg}' for #{f}, building configuration"
  coverity(cfg, f)
  commit(cfg)
end
