#!/usr/bin/env ruby
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

require 'optparse'
require 'pathname'
require 'logger'
require 'pp'

#################################################################
# Globals
#################################################################

$log = Logger.new(STDOUT)
$top_dir  = File.absolute_path("#{`pwd`}/..")
$build_dir = "#{$top_dir}/build"
$config_dir = "#{$build_dir}/configs"
$targets_file = "#{$top_dir}/build/make/templates/targets.in"
$subtargets_file = "#{$top_dir}/build/.subtargets.mk"
$profile_map = {}
$options = {}

#################################################################
# Structures for makefile information
#################################################################

PackageConfig = Struct.new("PackageConfig", :filename, :package, :multi, :profiles) do
    def to_s
        variant = "multi" if multi
        return "%-50s: %-10s %-5s" % [get_relative_to_build(), package.upcase, variant]
    end
    def config_command
        "#{$PROGRAM_NAME} -c #{get_relative_to_build()}"
    end
    def get_relative_to_build()
        return Pathname.new(filename).relative_path_from(Pathname.new($build_dir))
    end
    def get_target_name
        ""
    end
end

ProfileInfo = Struct.new("ProfileInfo", :name, :tgtname, :dtso, :api) do
    def to_s
        return "%-25s %-20s %-20s %-20s" % [name, tgtname, dtso, api]
    end
    def config_command(config_path)
        "#{$PROGRAM_NAME} -c #{config_path}"
    end
    def get_target_name
        "#{tgtname}.elf"
    end
end

def profile_header()
    return "%-25s %-20s %-20s %-20s %-20s %-20s %-20s" % ["Profile Name", "Target name", "DTSO", "API", "Chip", "MEBA", "DTBs"]
end

MultiProfileInfo = Struct.new("MultiProfileInfo", :name, :tgtname, :dtso, :api, :chip, :meba, :dtb) do
    def to_s
        return "%-25s %-20s %-20s %-20s %-20s %-20s %-20s" % [name, tgtname, dtso, api, chip, meba, dtb]
    end
    def config_command(package, config_path)
        "#{$PROGRAM_NAME} -c #{config_path} -s #{get_target_name(package)}"
    end
    def get_target_name(package)
        "#{package}_#{name}"
    end
end

#################################################################
# Shell helper
#################################################################

def sys(cmd)
    $log.info(cmd)
    system(cmd)
    if $? != 0
        raise "Running '#{cmd}' failed"
    end
end

#################################################################
# Helper functions for parsing makefiles
#################################################################

def get_config_files()
    return Dir.glob($config_dir + "/*.mk").sort()
end

def get_relative_to_build(filename)
    return Pathname.new(filename).relative_path_from(Pathname.new($build_dir))
end

def parse_targets_configuration(filename)
    $log.debug("parse_targets_configuration: #{filename}")
    profile_map = {}
    regex = /DefineNamedTarget,(\w+),\s+(\w+),\s+(\w+),\s+(\w+),\s+(\w+),\s+(\w+),\s+(\w+)/
    File.open(filename, "r").readlines().each do |line|
        if line =~ /DefineNamedTarget/
            matches = regex.match(line)
            if matches and matches.length >= 1
                profile_map[matches[1]] = MultiProfileInfo.new(*matches[1..-1])
            end
        end
    end
    return profile_map
end

def parse_configuration_file(filename)
    if not File.exist? filename
        puts "Error: Could not find the file: #{filename}"
        exit 1
    end
    $log.debug("parse_configuration_file: #{filename}")
    f = File.basename(filename)

    if f.match(/^([^_]+)?.*_multi.mk$/)
        # For multi files, everything until first "_" is the package type.
        pkg   = $1
        multi = true
    elsif f.match(/^([^_]+)/)
        # For non-multi files, the part before the first underscore is the
        # package type
        pkg   = $1
        multi = false
    else
        puts "Error: Filename (#{f}} doesn't meet match criteria"
        exit 1
    end
    $log.debug("File = #{f} => Package = #{pkg}, multi = #{multi}")

    package = PackageConfig.new(filename, pkg, multi, [])
    in_profile_list = false
    File.open(filename, "r").readlines().each do |line|
        if in_profile_list
            if line =~ /^\s*(\w+)\s*\\*$/
                package.profiles.push(ProfileInfo.new($1, nil, nil, nil))
            end
        end
        if line =~ /^\$.*DefineTarget,(\w+),(\w+),(\w+),(\w+)/
            package.profiles.push(ProfileInfo.new($1, $2, $3, $4))
        end
        if line =~ /^\$.*DefineTargetByPackage,(\w+),(\w+)/
            package.profiles.push(ProfileInfo.new("#{$1}_#{$2}", nil, nil, nil))
        end
        if line =~ /^\$.*DefineTargetByPackageNameApi,(\w+),(\w+),(\w+)/
            package.profiles.push(ProfileInfo.new("#{$1}_#{$2}", nil, nil, $3))
        end
        if line =~ /^\S+_PROFILE_LIST\s+=/
            in_profile_list = true
        end
    end
    return package
end

def show_detailed_build_information()
    get_config_files().each do |f|
        config = parse_configuration_file(f)
        puts "#{config.package.upcase()} package in #{f}"
        puts "    #{profile_header()}"
        config.profiles.each do |profile|
            if config.multi
                puts "    #{$profile_map[profile.name]}"
            else
                puts "    #{profile}"
            end
        end
    end
end

def show_package_configuration_help()
    packages = {}
    get_config_files().each do |f|
        config = parse_configuration_file(f)
        configs = packages[config.package.upcase()]
        if configs.nil?
            configs = [ config ]
        else
            configs.push(config)
        end
        packages[config.package.upcase()] = configs
    end
    packages.each do |package, configs|
        puts "#{package} package"
        configs.each do |package_config|
            puts "    #{package_config.config_command}"
        end
    end
end

def get_package_subtargets(f)
    result = []
    config = parse_configuration_file(f)
    if config.multi
        config.profiles.each do |profile|
            $log.debug("Get #{profile.name}")
            result.push($profile_map[profile.name].get_target_name(config.package))
        end
    else
        result.push(config.get_target_name)
    end
    return result
end

def show_configuration_help()
    get_config_files().each do |f|
        config = parse_configuration_file(f)
        relative_config_path = config.get_relative_to_build()
        puts "    #{config.config_command}"
        if config.multi
            config.profiles.each do |profile|
                $log.debug("Get #{profile.name}")
                puts "    #{$profile_map[profile.name].config_command(config.package, relative_config_path)}"
            end
        end
    end
end

def show_map()
    puts "    #{profile_header()}"
    $profile_map.each do |key, value|
        puts("    #{value}")
    end
end

def show_configuration_files()
    get_config_files().each do |f|
        config = parse_configuration_file(f)
        puts("    #{config}")
        config.profiles.each do |p|
            puts("          * #{p.name}")
        end
    end
end

#################################################################
# Command Line Options
#################################################################

$options[:subtargets] = []

opt_parser = OptionParser.new do |opts|
    opts.banner = """Usage: set_build_config [options]

    The default is to show the list of available configuration commands for all targets

Options:"""
    opts.on("-c", "--configuration path", "Create link to configuration file") do |d|
        $options[:config] = d
    end

    opts.on("-s", "--subtargets \"list\"", "Create a list of build subtargets") do |d|
        $options[:subtargets] << d
    end

    opts.on("-a", "--allsubtargets", "Create a list of all build subtargets") do |d|
        $options[:allsubtargets] = d
    end

    opts.on("-d", "--detailed", "Show a list of detailed subtargets information for all packages") do |d|
        $options[:detailed] = d
    end

    opts.on("-p", "--packages", "Show configuration by package") do |d|
        $options[:package] = d
    end

    opts.on("-x", "--clear", "Clear configuration") do |d|
        $options[:clear] = d
    end

    $options[:logging] = :error
    opts.on("-v", "--logging  <lvl>", [:info, :debug, :error], "Set log level.") do |d|
        $options[:logging] = d
    end

    opts.on("-h", "--help", "Show this message") do
        puts opts
        exit
    end
end

opt_parser.parse!(ARGV)

#################################################################
# Setup logging
#################################################################

case $options[:logging]
when :debug
    $log.level = Logger::DEBUG
when :info
    $log.level = Logger::INFO
else
    $log.level = Logger::ERROR
end

#################################################################
# Action
#################################################################

$profile_map = parse_targets_configuration($targets_file)
if $log.level == Logger::INFO
    show_map()
    show_configuration_files()
end

if $options[:detailed]
    show_detailed_build_information()
    exit
end

if $options[:package]
    show_package_configuration_help()
    exit
end

if $options[:clear]
    puts "Remove config.mk"
    sys("rm -f config.mk")
    puts "Remove #{get_relative_to_build($subtargets_file)}"
    sys("rm -f #{$subtargets_file}")
    exit
end

if $options[:allsubtargets] and $options[:config]
    $options[:subtargets] = get_package_subtargets($options[:config])
end

if not $options[:subtargets].empty?
    all_subtargets = get_package_subtargets($options[:config])
    $options[:subtargets].each do |tgt|
        if not all_subtargets.include? tgt
            puts "Error: #{tgt} is not a valid subtarget"
            exit 1
        end
    end
    puts "Create subtargets in #{get_relative_to_build($subtargets_file)}"
    open($subtargets_file, 'w') do |f|
        all_subtargets.each do |tgt|
            if $options[:subtargets].include? tgt
                f.puts("BUILD_SUBTARGETS   += #{tgt}")
            else
                f.puts("# BUILD_SUBTARGETS   += #{tgt}")
            end
        end
    end
end

if $options[:config]
    puts "Create link to #{$options[:config]}"
    if not File.exist? $options[:config]
        puts "Error: File does not exist"
        exit 1
    end
    sys("ln -sf #{$options[:config]} config.mk")
end

if $options[:config] and $options[:subtargets].empty?
    puts "Remove #{get_relative_to_build($subtargets_file)}"
    sys("rm -f #{$subtargets_file}")
    exit
end

if $options[:config] or not $options[:subtargets].empty?
    exit
end

show_configuration_help()
