#!/usr/bin/env ruby
# -*- coding: iso-8859-1 -*-
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

require_relative 'chk_scripts_shared.rb'
require 'yaml'

$sysCmd = SysCmd.new
startTime = Time.now
#################################################################
# Input parser
#################################################################
require 'optparse' # For command line parsing

$options = {}
global = OptionParser.new do |opts|
  opts.banner = "Usage: run_checks.rb [options]"
  opts.version = 0.1

  $options[:verbose] = :debug
  opts.on("-v", "--verbose  <lvl>", [:error, :info, :debug], "Set verbose level.") do |lvl|
    $options[:verbose] = lvl
  end

  $options[:cfgDir] = "configs"
  opts.on("-c", "--cfg-dir  <dir>", "Path to directory containing configurations. It is relative to the build directory.") do |dir|
    $options[:cfgDir] = dir
  end

  opts.on("-i", "--include-cfg-list  <list>", "Space separated list of configurations to include. Regexp greedy - e.g. \"web bringup\".") do |list|
    $options[:list] = list
  end

  opts.on("-e", "--exclude-cfg-list  <list>", "Space separated list of configurations to exclude. Regexp greedy - e.g. \"web bringup\".") do |list|
    $options[:exclude] = list
  end

  $options[:doxygenChk] = true
  opts.on("-d", "--skip-doxy-chk", "Do not perform doxygen check.") do
    $options[:doxygenChk] = false
  end

  opts.on("-r", "--skip-rs1014-chk", "Do not perform rs1014 check.") do
    $options[:rs1014skip] = true
  end

  opts.on("-l", "--skip-licenses-chk", "Do not perform licenses check.") do
    $options[:licensesskip] = true
  end

end.order!


#################################################################
#Setup the trace system
#################################################################
$v= MyLogger.new (STDOUT)

case $options[:verbose]
when :info
  $v.level = Logger::INFO
  $v.info("Info level enabled")
when :debug
  $v.level = Logger::DEBUG
else
  $v.level = Logger::ERROR
end

#################################################################
# Getting which configurtions to check and which specific
# configurations not to check
#################################################################
$cfgsSkipped = []
def skipCfg (cfg)
  skip = false
  # Checking if user only wants to exclude some specific configurations
  if !$options[:exclude].nil?
    inList = false
    list = $options[:exclude].split(" ")

    list.each do |l|
      if !cfg.match(l.strip).nil?
        inList = true
      end
    end

    if inList
      $v.info("Skipping configuration: #{cfg} per user request")
      skip = true
    end
  end

  # Checking if user only wants some specific configurations
  if !$options[:list].nil?
    inList = false
    list = $options[:list].split(" ")

    list.each do |l|
      if !cfg.match(l.strip).nil?
        inList = true
      end
    end

    if !inList
      $v.info("Skipping configuration: #{cfg} per user request")
      skip = true
    end
  end

  if skip
    $cfgsSkipped.push(cfg)
    return true
  end

  # OK - This shall not be skipped
  return false
end

# Get list of configurations to do the checks for
def cfgsGet buildDir
  # As default configurations shall be checked
  configs = $sysCmd.cmd("ls #{buildDir}/#{$options[:cfgDir]}").split("\n")

  # Remove some selected configurations
  configs.each do |cfg|
    if skipCfg(cfg)
      configs -= [cfg]
    else
      $v.info("Adding configuration: #{cfg}")
    end
  end
  return configs.join(",")
end

#################################################################
# Setup
#################################################################

# Directories
srcDir = %x(git rev-parse --show-toplevel).strip!
buildDir = "#{srcDir}/build"
objDir = "#{buildDir}/obj"

#################################################################
# Script start
#################################################################
$sysCmd.cmd("rm -rf #{objDir}; mkdir -p #{objDir}")
configs = cfgsGet(buildDir)

# Do the checking
cmd = "./code_static_chks.rb  --src-dir #{srcDir} --cfg '#{configs}' --build-dir build"
cmd += " --json-status #{srcDir}/build/obj/results/status.json"
cmd += " --verbose #{$options[:verbose]}"
cmd += " --cfgs-skipped \"#{$cfgsSkipped.join(',')}\""
cmd += " --skip-doxy-chk" if !$options[:doxygenChk]
cmd += " --skip-rs1014-chk" if $options[:rs1014skip]
cmd += " --skip-licenses-chk" if $options[:licensesskip]
cmd += " --skip-release-note" # Never generate release note on ordinary compilations

$v.debug(cmd)
puts $sysCmd.cmd(cmd)

# Runtime
runTime = Time.now - startTime
$v.info("Checking took:#{runTime} sec.")

exit $sysCmd.anyErrors
