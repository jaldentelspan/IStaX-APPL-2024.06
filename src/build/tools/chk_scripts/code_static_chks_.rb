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

require 'pp'
require 'yaml'
require 'logger'
require 'optparse'
require 'fileutils'
require_relative './resultnode.rb'

$conf = {}
OptionParser.new do |opts|
  opts.on("--conf <config>", "") do |c|
      $conf[:target] = c
  end

  opts.on("--steps <s>", "") do |s|
      $conf[:steps] = s.split(",")
  end
end.order!

begin
    $top_dir = Dir.pwd

    $time_start = Time.now
    Dir.chdir("build") ############!!!!!! EVERYTHING HAPPENS RELATIVE TO BUILD FROM NOW
    FileUtils.rm_rf "obj"
    FileUtils.rm_rf "config.mk"
    File.symlink("configs/#{$conf[:target]}", "config.mk")
    FileUtils.mkdir_p "obj/results"

    $l = Logger.new("obj/results/sg_build_log.txt")
    $l.level = Logger::DEBUG

    $l.debug $conf
    target = $conf[:target]
    target.gsub! /\.mk$/, ""
    $top = ResultNode.new(target, "OK", {})
    $supported_steps = []

rescue => e
    s = "Fatal error when initializing!!!\n"
    s += e.backtrace.join("\n\t").sub("\n\t", ": #{e}#{e.class ? " (#{e.class})" : ''}\n\t")
    s += "\n--------------------\n"

    $top = ResultNode.new($conf[:target], "Fatal error when initializing", {:msg => s})
    $top.to_file "obj/results/status.json"

    exit 1
end

def sh step, cmd
    $l.debug "#{step}: #{cmd}"
    system(cmd)
    if $?.exitstatus != 0
        puts "#{step}: FAILED #{cmd} -> #{$?.exitstatus}"
        $l.info "#{step}: FAILED #{cmd} -> #{$?.exitstatus}"
        raise "Command failed"
    end
end

# Returns the result of a test based on if the stderr file contains anything.
# The return type is "ResultNode"
def stderrNonZeroResult step
    if File.zero?("obj/results/#{step}_stderr.txt")
        return ResultNode.new(step, "OK", {})
    else
        return ResultNode.new(step, "Failed", {})
    end
end

def cap step, cmd
    c = "#{cmd} >> obj/results/#{step}_stdout.txt 2>> obj/results/#{step}_stderr.txt"
    sh step, c
    return stderrNonZeroResult(step)
end

###############################################################################
def step_compile step
    st = ResultNode.new("", "OK", {})
    st.addSibling cap(step, "make -j12")

    system "cp obj/cmd_ref.htm obj/results/."

    # Dig out meta-data from the compilation
    show_modules = %x{make show_modules}

    # $ make show_modules
    # OS: Linux
    # Platform: ISTAX IStaX   UNDEF   BOARD_UNDEF     STANDALONE
    # External-toolchain: /opt/mscc/mscc-brsdk-mipsel-2018.11.2-191
    # External-mesa: mesa-5.8.0-SOAK-198-g689cc86@4-dev.fa
    # Enabled: access_mgmt acl afi aggr alarm arp_inspection auth backtrace board build_istax build_smb cdp cli cli_telnet conf daylight_saving ddmi dhcp6_client dhcp6_relay dhcp6_snooping dhcp_client dhcp_helper dhcp_relay dhcp_server dhcp_snooping dns dot1x dot1x_acct eee eth_link_oam fan fast_cgi firmware frr garp gvrp icfg icli ip ip_misc ip_source_guard ipmc ipmc_lib ipv6 ipv6_source_guard json_ipc json_rpc json_rpc_notification l2proto l3rt lacp led_pow_reduc lldp lldp_med loop_detect loop_protection mac main md5 meba mirror misc mrp msg mstp mvr mvrp nas nas_dot1x_multi nas_dot1x_single nas_mac_based nas_options ntp optional_modules p802_1_as packet phy poe port port_power_savings priv_lvl psec psec_limit ptp pvlan qos radius rmirror rmon sflow smb_ipmc smb_snmp smon snmp sprout ssh subject symreg synce synce_dpll syslog sysutil tacplus thermal_protect thread_load_monitor timer tod udld upnp users util vcl vlan vlan_translation voice_vlan vtss_api vtss_basics web webcontent xxrp zls30387
    # Disabled: aps erps errdisable json_demo loop_protect phy_warm_start_test post private_mib private_mib_gen sr sw_push_button vtss_appl warm_start

    toolchain = nil
    show_modules.each_line do |l|
        case l
        when /Using toolchain:\s*(.*)/
            m = $1
            (x, arch, stage2) = m.split /[\s-]+/
            st.meta["arch"] = arch
            st.meta["stage2"] = stage2

        when /OS:\s*(.*)/
            st.meta["os"] = $1

        when /Platform:\s*(.*)/
            st.meta["platform"] = $1.split /\t/

        when /External-toolchain:\s*(.*)/
          bsp_path = $1
            st.meta["bsp"] = bsp_path
            $l.debug "BSP: #{bsp_path}"

            begin
                f = YAML.load_file("#{bsp_path}/.mscc-version")
                st.meta["bsp_version"] = f["version"]
                st.meta["bsp_revision"] = f["revision"]
                st.meta["bsp_bin_id"] = f["build_id"]
                $l.debug "Build ids: #{f["version"]} #{f["revision"]} #{f["build_id"]}"
            rescue => e
                msg = "Exception #{$conf[:target]}/#{step.to_s}\n"
                #msg += e.backtrace.join("\n\t").sub("\n\t", ": #{e}#{e.class ? " (#{e.class})" : ''}\n\t")
                msg += "\n--------------------\n"
                puts msg
                $l.error msg
                $l.error show_modules
            end

        when /Enabled:\s*(.*)/
            st.meta["module_enable"] = $1.split /\s/

        when /Disabled:\s*(.*)/
            st.meta["module_disable"] = $1.split /\s/
        else
        end
    end

    # Pull API version and influde in JSON file
    api_version = ""
    api_branch = ""
    File.readlines('make/paths-api.mk').each do |l|
      case l
      when /^MESA_API_VERSION\s*\?=\s*(\S+)/
        api_version = $1
      when /^MESA_API_BRANCH\s*\?=\s*(\S+)/
        api_branch = $1
      end
    end
    st.meta["api_version_and_branch"] = "#{api_version}@#{api_branch}"

    target = $conf[:target]
    %w(itb ext4.gz ubifs mfi).each do |imgtype|
      Dir.glob("obj/*.#{imgtype}").each do |f|
        $l.info("tgt:#{target} image:#{f}")
        system "cp -v #{f} obj/results/."
      end
    end

    return st
end

###############################################################################
def step_mibs step
    o = "obj/results/mibs.tar"
    st = cap(step, "./release/mib_release.rb --dir .. --output-file #{o}")
    if File.exist? o
        sh(step, "gzip #{o}")
    end
    return st
end

###############################################################################
def step_tidy step
    return cap(step, "make tidy -j12")
end

###############################################################################
def step_public_hdr step
    return cap(step, "make hdr_check")
end

###############################################################################
def step_doxygen step
    return cap(step, "make -j12 doxygen")
end

###############################################################################
def step_doxygen_no_pdf step
    return cap(step, "make -j12 doxygen-no-pdf")
end

###############################################################################
def step_jslint step
    return cap(step, "make -j12 jslint")
end

###############################################################################
def step_codestyle step
    return cap(step, "make -j12 code_style_chk CLEANUP=1")
end

###############################################################################
def step_ctidy step
    return cap(step, "make -j12 ctidy")
end

###############################################################################
def step_run s
    $l.debug "Step: #{s}"
    st = nil
    found = false

    begin
        case s
        when "compile"
            st = step_compile s

        when "mibs_check"
            st = step_mibs s

        when "tidy"
            st = step_tidy s

        when "public_hdr"
            st = step_public_hdr s

        when "doxygen"
            st = step_doxygen s

        when "doxygen_no_pdf"
            st = step_doxygen_no_pdf s

        when "jslint"
            st = step_jslint s

        when "codestyle"
            st = step_codestyle s

        when "ctidy"
            st = step_ctidy s

            ### ADD NEW STEPS HERE ############################################
        else
            $l.debug "Step: #{s} not implemented"
            st = ResultNode.new(s.to_s, "Not implemented", {})
        end

        st.name = s.to_s

    rescue => e
        msg = "Exception #{$conf[:target]}/#{s.to_s}\n"
        msg += e.backtrace.join("\n\t").sub("\n\t", ": #{e}#{e.class ? " (#{e.class})" : ''}\n\t")
        msg += "\n--------------------\n"
        puts msg
        $l.debug msg
        st = ResultNode.new(s.to_s, "FAILED", {})
    end

    $l.debug "Step: #{s} #{st.status}"
    $top.addSibling st
end

begin
    threads = []

    # Doxygen takes for ever to run - get it started in its own thread as the
    # first thing
    if $conf[:steps].include? "doxygen"
        $conf[:steps].delete "doxygen"

        threads << Thread.new do
            step_run "doxygen"
        end
    end

    # Some steps depends on a completed compilation, so do that first and then
    # continue with the remaining steps
    if $conf[:steps].include? "compile"
        $conf[:steps].delete "compile"
        step_run "compile"
    end

    threads << Thread.new do
        $conf[:steps].each do |s|
            step_run s
        end
    end

    threads.map(&:join)

rescue => e
    s = "Exception #{$conf[:target]}\n"
    s += e.backtrace.join("\n\t").sub("\n\t", ": #{e}#{e.class ? " (#{e.class})" : ''}\n\t")
    s += "\n--------------------\n"
    puts s
    $l.debug s
    $top = ResultNode.new($conf[:target], "Fatal error when processing steps", {:msg => s})
end

begin
    $time_stop = Time.now
    $top.meta["time_compile"] = ($time_stop - $time_start).to_i
rescue
    s = "Exception #{$conf[:target]}\n"
    s += e.backtrace.join("\n\t").sub("\n\t", ": #{e}#{e.class ? " (#{e.class})" : ''}\n\t")
    puts s
    $l.debug s
end

$top.to_file "obj/results/status.json"
