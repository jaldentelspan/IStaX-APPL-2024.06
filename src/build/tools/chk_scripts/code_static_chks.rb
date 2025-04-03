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

require 'yaml'
require 'thread'
require 'logger'
require 'optparse'
require 'fileutils'
require 'pp'
require 'open3' # For capturing stdout, stderr, and status in one go.
require_relative './results_table.rb'
require_relative './resultnode.rb'

$steps = [:compile, :mibs_check, :tidy, :public_hdr, :doxygen, :doxygen_no_pdf, :jslint, :ctidy, :codestyle]
$chkGlobalTypes = [:copyrights, :smi_lint, :licenses, :release_note, :rs1014]
$sem = Mutex.new
$jenkinsArtifact = "artifact/build/obj/results"
$options = {}
global = OptionParser.new do |opts|
  opts.banner = "Usage: release_compile"
  opts.version = 0.1

  opts.on("-t", "--skip-tidy-chk", "Do not perform tidy check.") do
      $steps.delete :tidy
  end

  opts.on("-c", "--skip-code-style-chk", "Do not perform code style check.") do
      $steps.delete :codestyle
  end

  opts.on("-h", "--skip-hdr-chk", "Do not perform public header check.") do
      $steps.delete :public_hdr
  end

  opts.on("-m", "--skip-smi-chk", "Skip SMI Lint check.") do
      $steps.delete :mibs_check
  end

  opts.on("-j", "--skip-jsl-chk", "Do not perform JavaSrcipt Lint check.") do
      $steps.delete :jslint
  end

  opts.on("-j", "--skip-ctidy-chk", "Do not perform ctidy check.") do
      $steps.delete :ctidy
  end

  opts.on("-d", "--skip-doxy-chk", "Do not perform doxygen check.") do
      $steps.delete :doxygen
  end

  opts.on("--compile-only", "Skip all steps except for compilation") do
      $steps = [:compile]
      $chkGlobalTypes = []
  end

  opts.on("-r", "--skip-rs1014-chk", "Do not perform rs1014 check (default if not skipping release-note generation, because that one embeds an RS1014 check. See also --no-rs1014)") do
      $chkGlobalTypes.delete :rs1014
  end

  $options[:no_rs1014] = ""
  opts.on("--no-rs1014", "Like --skip-rs1014-chk, but also avoids using RS1014 when generating release-note (useful for .snapshot branches)") do
      $chkGlobalTypes.delete :rs1014
      $options[:no_rs1014] = "--no-rs1014"
  end

  opts.on("-n", "--skip-release-note", "Do not generate release-note.") do
      $chkGlobalTypes.delete :release_note
  end

  opts.on("--skip-licenses-chk", "Do not perform licenses check.") do
      $chkGlobalTypes.delete :licenses
  end

  opts.on("-s", "--src-dir <dir>", "Path to where to take the source code from.") do |dir|
      $src_dir = dir
  end

  opts.on("-f", "--cfg <files>", "Comma seperated list of configuration files.") do |files|
      $configs = files.split(",")
  end

  $configs_skipped = []
  opts.on("-o", "--cfgs-skipped <files>", "Comma seperated list of configuration files that is not checked.") do |files|
      $configs_skipped = files.split(",")
  end

  opts.on("--release-version <num>", "Version string to diplay to the user") do |num|
      $options[:relString] = num
  end

  $build_dir_rel = "build"
  opts.on("-b", "--build-dir <dir>", "Path to the build directory.") do |dir|
      $build_dir_rel = dir
  end

  opts.on("--log-file <file>", "Log file") do |f|
      $options[:log_file] = f
  end

  opts.on("--quiet", "Do not log to stdout (default is to log to stdout and log file if specified)") do |f|
      $options[:quiet] = true
  end

  opts.on("--json-status <file>", "Where to write the JSON status file") do |f|
      $options[:json_status] = f
  end

  $options[:loglevel] = Logger::INFO
  opts.on("-v", "--verbose <level>", [:error, :warn, :info, :debug], "Set log level.") do |l|
      $options[:loglevel] = {:error => Logger::ERROR, :warn => Logger::WARN, :info => Logger::INFO, :debug => Logger::DEBUG}[l]
  end

  opts.on("-u", "--jenkins-build-url  <url>", "Jenkins build URL environment variables (BUILD_URL).") do |url|
    $options[:url] = url
  end

  $options[:sg_port] = 30001
  opts.on("--sg-port <port>", "Simple grid port") do |i|
      $options[:sg_port] = i
  end

  $options[:no_globals] = false
  opts.on("--no-globals", "Disable global checks") do
      $options[:no_globals] = true
  end

  $options[:only_globals] = false
  opts.on("--only-globals", "Only globals") do
      $options[:only_globals] = true
  end
end.order!

# If generating a release-note, no need to perform RS1014 check, since that's
# embedded in the release-note generation.
if $chkGlobalTypes.include? :release_note
    $chkGlobalTypes.delete :rs1014
end

# Create root nodes - leave non-used dangling
$st_top = ResultNode.new("webstax", "OK", {})
$st_globals = ResultNode.new("globals", "OK", {})
$st_configs = ResultNode.new("configs", "OK", {})

if not $options[:only_globals]
    $st_top.addSibling($st_configs)

    if not $options[:no_globals]
        $st_top.addSibling($st_globals)
    end
end

if $configs.nil?
    puts "No configs specified!"
    exit 1
end

if $src_dir.nil?
    puts "No src-dir!"
    exit 1
end

# Also skip licenses check if .../build/release/licenses.rb doesn't exist,
# because this is released software, which doesn't include the licenses.rb
# script.
if not File.directory?($src_dir)
   $chkGlobalTypes.delete :licenses
end

if $options[:log_file]
    if $options[:quiet]
        $l = Logger.new("#{$options[:log_file]}")
    else
        $l = Logger.new("| tee #{$options[:log_file]}")
    end
else
    if $options[:quiet]
        $l = Logger.new("/dev/null")
    else
        $l = Logger.new(STDOUT)
    end
end
$l.level = $options[:loglevel]


# Delete skipped configs from the config string
$configs = $configs - $configs_skipped
$configs_including_skipped = $configs + $configs_skipped

$configs.uniq!
$configs_skipped.uniq!
$configs_including_skipped.uniq!

$results_dir_rel = "#{$build_dir_rel}/obj/results"
$results_dir = "#{$src_dir}/#{$results_dir_rel}"
FileUtils.mkdir_p $results_dir
$bsp_bin_id = []
$bsp_revision = []
$api_name = []

$l.debug "src: #{$src_dir} res: #{$results_dir}"

# If local build, point to local results
# $jenkinsArtifact = "#{$src_dir}/build/obj/results"

###############################################################################
# Global steps are done here

# DO NOT CHANGE DIR
# DO NOT ACCESS GLOBALS WITHOUT "$sem.synchronize { }" BLOCK
# DO NOT PRINT TO STDOUT/STDERR USE $l.debug INSTEAD

def sh step, cmd
    $l.debug "#{step}: #{cmd}"
    system(cmd)
    if $?.exitstatus != 0
        puts "#{step}: FAILED #{cmd} -> #{$?.exitstatus}"
        $l.info "#{step}: FAILED #{cmd} -> #{$?.exitstatus}"
        raise "Command failed"
    end
end

def cap step, cmd
    system "mkdir -p #{$src_dir}/build/obj/results/globals"
    c = "#{cmd} >  #{$src_dir}/build/obj/results/globals/#{step}_stdout.txt 2>  #{$src_dir}/build/obj/results/globals/#{step}_stderr.txt"
    sh step, c
end

# Returns the result of a test based on if the stderr file contains anything.
# The return type is "ResultNode"
def stderrNonZeroResult step
    stderr_file = "#{$src_dir}/build/obj/results/globals/#{step}_stderr.txt"
    if File.zero?(stderr_file)
        return ResultNode.new(step, "OK", {})
    else
        node = ResultNode.new(step, "Failed", {})
        File.readlines(stderr_file).each do |line|
            node.addSibling(ResultNode.new(line, "Failed"))
        end
        return node
    end
end

# Things needed before we can run the global checks
def globalPrepare
    # Link to a configuration. Any will do. Just use the first one
    %x(rm -rf #{$src_dir}/build/config.mk) # Just in case it already exists
    %x(ln -s #{$src_dir}/build/configs/#{$configs[0]}  #{$src_dir}/build/config.mk)
end

def step_global_copyrights step
    cap(step, "make -C #{$src_dir}/build/ copyrights")
end

def step_global_smi_lint step
    cap(step, "make -C #{$src_dir}/build smilint")
end

def step_global_release_note step
    # Create release note, which at the same time performs an RS1014 check,
    # because the release-note generator script needs to invoke the RS1014
    # script itself in order to generate a list of new features. Doing so also
    # generates HTML feature lists and overview.
    $l.info("Creating release note")
    cap(step, "#{$src_dir}/build/release/release_note.rb -v info #{$options[:no_rs1014]}")

    # Don't know where to put the following if it ain't gonna be here. It has
    # nothing to do with the release note; it's just additional output.

    # Copy HTML gloassary.
    %x(cp #{$src_dir}/vtss_appl/web/html/help/glossary.htm #{$results_dir}/.)
end

def step_global_rs1014 step
    $l.info("Making RS1014 check")
    cap(step, "make -C #{$src_dir}/build rs1014")
end

def step_global_licenses step
    $l.info("Making licenses check")
    cap(step, "make -C #{$src_dir}/build licenses_check")
end

def process_global_step s
    $l.debug "Global step: #{s} start"
    st = nil

    begin
        case s
        when :copyrights
            step_global_copyrights s
        when :smi_lint
            step_global_smi_lint s
        when :licenses
            step_global_licenses s
        when :release_note
            step_global_release_note s
        when :rs1014
            step_global_rs1014 s
        else
            raise "Global step: #{s.to_s} not implemented"
        end
        st = stderrNonZeroResult(s)
        st.name = s.to_s # do not move to ensure as we want to catch it st.nil?
    rescue => e
        msg = "Global step: #{s} failed:\n"
        msg += e.backtrace.join("\n\t").sub("\n\t", ": #{e}#{e.class ? " (#{e.class})" : ''}\n\t")
        puts msg
        $l.error msg

        st = ResultNode.new(s.to_s, "Failed", {})
    end

    $l.debug "Global step: #{s.to_s} done: #{st.status}"
    $sem.synchronize {
        $st_globals.addSibling(st)
    }
end

def process_global_steps
    $chkGlobalTypes.each do |s|
        process_global_step s
    end
end


###############################################################################
def process_config c
    dir = "#{$results_dir_rel}/#{c}"
    FileUtils.rm_rf "#{$src_dir}/#{dir}"
    dir_local  = "#{$src_dir}/#{dir}/local"
    dir_result = "#{$src_dir}/#{dir}/result"

    FileUtils.mkdir_p dir_local
    FileUtils.mkdir_p dir_result

    code_rev = %x{#{$src_dir}/build/tools/code_version}.strip

    cmd = "hostname; whoami; pwd; sudo /usr/local/bin/mscc-cleanup; ./build/tools/chk_scripts/code_static_chks_.rb --conf #{c} --steps #{$steps.join(",")}"
    sg  = "SimpleGridClient"
    sg += " -e MCHP_DOCKER_NAME=\"ghcr.io/microchip-ung/bsp-buildenv\" -e MCHP_DOCKER_TAG=\"1.19\""
    sg += " --concurrency 50"
    sg += " --meta type=webstax --meta config=#{c}"
    sg += " --stamps #{dir_local}"
    sg += " --log #{dir_local}/sg_client.log"
    sg += " -e CODE_REVISION=#{code_rev}"
    sg += " -e PATH=/usr/local/bin:/bin:/usr/bin:/usr/sbin"
    sg += " -e WS2_IMAGE_SIZE_CHK=1"
    if $options[:relString]
        sg += " -e RELEASE_VERSION='#{$options[:relString]}'"

        # Because RELEASE_VERSION may contain spaces, we have to disable docker
        # on the simple grid servers, because there is a bug in one or more of
        # the scripts that causes the quotes to be lost before injected into the
        # docker container. Setting the following two environment variables to
        # empty strings will cause docker to be disabled:
        # sg += " -e MCHP_DOCKER_NAME=\"\" -e MCHP_DOCKER_TAG=\"\""
    end
    sg += " -e BUILD_NUMBER"
    sg += " -l webstax"
    sg += " -w #{$src_dir}/build/obj/ws.tar" # Workspace to transfer to blade
    sg += " -c '#{cmd}'" # Command to run on blade
    sg += " -a ./build/obj/results"  # Folder to archive (on blade)
    sg += " -o #{$src_dir}/build/obj/#{c}.tar" # Where to put the archive (on server)

    $l.info "SimpleGrid Start: #{c}"
    $l.info "#{sg}"
    time_sg_start = Time.now
    stdout, stderr, status = Open3.capture3(sg)
    time_sg_end = Time.now

    if !status.success?
        $l.error("Error: When executing \"#{sg}\": #{stderr}")
        $l.error("SimpleGridClient's stdout: #{stdout}")

        # Try to figure out what server caused the issue
        server = "unknown"
        begin
            server = IO.read("#{dir_local}/to_blade.remote_server")
        rescue => e
        end

        $l.error "#{c} SG exit code: #{$?.exitstatus} server: #{server}"
        raise "#{c} SG exit code: #{$?.exitstatus} server: #{server}"
    else
        $l.info("SimpleGridClient's stdout: #{stdout}")
    end

    sh "WS-#{c}", "tar -C #{dir_result} --strip-components=4 -xf #{$src_dir}/build/obj/#{c}.tar"

    time_total = (time_sg_end - time_sg_start).to_i

    # Remote processing is done, and we can start to aggregate the result.
    # This is supposed to throw an exception if the status file could not be
    # found!
    st = ResultNode.from_file "#{dir_result}/status.json"

    # Calculate how much time was spend on waiting, transfer (net) and compiling.
    begin
        ts_start = File.stat("#{dir_local}/sg-start.stamp").mtime
        ts_offer = File.stat("#{dir_local}/sg-offer.stamp").mtime
        ts_stop = File.stat("#{dir_local}/sg-stop.stamp").mtime
        server = IO.read("#{dir_local}/sg-offer.stamp")
        st.meta["remote_server"] = server

        time_compile = st.q("time_compile")
        time_net = (ts_stop - ts_offer).to_i - time_compile
        time_wait = (ts_offer - ts_start).to_i
        st.meta["time_net"] = time_net
        st.meta["time_wait"] = time_wait
        $l.debug "#{c} Server: #{server} Total: #{time_total} Wait: #{time_wait} Network: #{time_net} Compile: #{time_compile}"

    rescue => e
        msg = e.backtrace.join("\n\t").sub("\n\t", ": #{e}#{e.class ? " (#{e.class})" : ''}\n\t")
        puts msg
        $l.error "#{c} Failed to calculate time usage"
        $l.error msg
    end

    $sem.synchronize {
        begin
            $bsp_bin_id += st.q("compile@bsp_bin_id")
            $bsp_revision += st.q("compile@bsp_revision")
            $api_name += st.q("compile@api_version_and_branch")
        rescue
        end
        $st_configs.addSibling(st)
    }
end

def config_type a
    case a
    when /^bringup_/
        return :bringup
    when /^web/
        return :web
    when /^smb/
        return :smb
    when /^istax/
        return :smb
    when /^ce/
        return :ce
    else
        return :unknown
    end
end

def config_sort_name a
    case a
    when /^bringup_/
        return "050-#{a}"
    when /^web/
        return "040-#{a}"
    when /^smb/
        return "030-#{a}"
    when /^istax/
        return "020-#{a}"
    when /^ce/
        return "010-#{a}"
    else
        return "999-#{a}"
    end
end

def process_parallel
    threads = []

    cnt = 0
    total = $configs.size

    if not $options[:no_globals]
        globalPrepare # Setups needed before multi threading the checks
        threads << Thread.new do
            process_global_steps
        end
    end

    $configs.sort! do |a, b|
        config_sort_name(a) <=> config_sort_name(b)
    end

    last_config = :unknown
    $configs.each do |c|
        threads << Thread.new do
            tries = 0
            not_done = true

            while not_done and tries < 1
                begin
                    tries += 1
                    process_config c

                    # If we changed "config" type, then sleep a few seconds to
                    # allow simple grid to allocate nodes for the first set (if
                    # avialable)
                    if last_config != :unknown and last_config != config_type(c)
                        sleep 3
                    end
                    last_config = config_type(c)

                    $sem.synchronize {
                        cnt += 1
                        $l.debug "#{c} Done #{cnt}/#{$configs.size} after #{tries} tries"
                    }

                    not_done = false

                rescue => e
                    msg = e.backtrace.join("\n\t").sub("\n\t", ": #{e}#{e.class ? " (#{e.class})" : ''}\n\t")
                    puts msg
                    $l.error "#{c} Config failed. Tries: #{tries}"
                    $l.error msg
                end
            end

            if not_done
                $sem.synchronize {
                    $st_configs.addSibling(ResultNode.new(c, "Failed after many tries!"))
                }
            end
        end
    end

    threads.map(&:join)
end

def add_res r, q, file = nil, add_color = false
    msg = nil
    color = nil

    # Point to Jenkins artifact
    file = "#{$jenkinsArtifact}/" + file if !file.nil?

    begin
        s = $st_top.q q
        if s == "OK"
            color = "green"
            msg = "OK"
            file += "_stdout.txt" if !file.nil?
        else
            color = "red"
            msg = s
            file += "_stderr.txt" if !file.nil?
        end
    rescue => e
        color = "yellow"
        msg = "skipped"
        file = nil
    end

    if add_color
        r.addCell(msg, file, color)
    else
        r.addCell(msg, file, "")
    end

    return msg
end

def make_reports
    c = ResultsMatrix.new()

    # Global status
    gt = ResultsTable.new()
    gt_ = [["Summary",     "globals@status",               nil],
           ["Copyright",   "globals.copyrights@status",    "globals/copyrights"],
           ["SmiLint",     "globals.smi_lint@status",      "globals/smi_lint"],
           ["Licenses",    "globals.licenses@status",      "globals/licenses"],
           ["ReleaseNote", "globals.release_note@status",  "globals/release_note"],
           ["RS1014",      "globals.rs1014@status",        "globals/rs1014"]]
    gt_.each do |x|
        r = ResultsRow.new()
        r.addCell(x[0], nil, "")
        add_res(r, x[1], x[2], true)
        gt.addRow r
    end

    gt.header.addCells(["Name", "Status"])
    c.addTable(gt)

    # Configuration status
    resultList = ResultsTable.new()
    chkTypes = ["Configuration", "Status", "Compile", "MibsCheck", "Tidy", "PublicHdr", "Doxygen", "DoxygenNoPDF", "JSLint", "ctidy", "CodeStyle", "Server", "Wait (sec)", "Net (secs)", "Compile (sec)"]
    summary = {}
    chkTypes.each do |e|
        summary[e] = []
    end

    # Adding a row for each configuration (based on the data from the json
    # status)
    $configs.each do |c|
        c.gsub! /\.mk$/, ""
        r = ResultsRow.new()

        r.addCell(c, "#{$jenkinsArtifact}/#{c}.mk/result")
        summary["Status"]        << add_res(r, "configs.#{c}@status", nil, true) # summary in color
        summary["Compile"]       << add_res(r, "configs.#{c}.compile@status", "#{c}.mk/result/compile")
        summary["MibsCheck"]     << add_res(r, "configs.#{c}.mibs_check@status", "#{c}.mk/result/mibs_check")
        summary["Tidy"]          << add_res(r, "configs.#{c}.tidy@status", "#{c}.mk/result/tidy")
        summary["PublicHdr"]     << add_res(r, "configs.#{c}.public_hdr@status", "#{c}.mk/result/public_hdr")
        summary["Doxygen"]       << add_res(r, "configs.#{c}.doxygen@status", "#{c}.mk/result/doxygen")
        summary["DoxygenNoPDF"]  << add_res(r, "configs.#{c}.doxygen_no_pdf@status", "#{c}.mk/result/doxygen_no_pdf")
        summary["JSLint"]        << add_res(r, "configs.#{c}.jslint@status", "#{c}.mk/result/jslint")
        summary["ctidy"]         << add_res(r, "configs.#{c}.ctidy@status", "#{c}.mk/result/ctidy")
        summary["CodeStyle"]     << add_res(r, "configs.#{c}.codestyle@status", "#{c}.mk/result/codestyle")
        summary["Server"]        << add_res(r, "configs.#{c}@remote_server")
        summary["Wait (sec)"]    << add_res(r, "configs.#{c}@time_wait")
        summary["Net (secs)"]    << add_res(r, "configs.#{c}@time_net")
        summary["Compile (sec)"] << add_res(r, "configs.#{c}@time_compile")

        resultList.addRow(r)
    end

    # Adding a summary row

    pp resultList
    puts resultList

    rr = ResultsRow.new()
    chkTypes.each do |e|
        case e
        when "Configuration"
            rr.addCell("##{$configs.size}", nil)
        when "Status", "Compile", "MibsCheck", "Tidy", "PublicHdr", "Doxygen", "DoxygenNoPDF", "JSLint", "ctidy", "CodeStyle"
            cnt_not_ok = summary[e].count{|x| x != "OK"}

            u = summary[e].uniq
            if u.size == 1 and u[0] == "OK"
                rr.addCell("OK", nil, "green")
            elsif u.size == 1 and u[0] == "skipped"
                rr.addCell("skipped", nil, "yellow")
            elsif u.size == 1
                rr.addCell(u[0], nil, "red")
            else
                rr.addCell("Failed #{cnt_not_ok}/#{$configs.size}", nil, "red")
            end

        when "Server"
            rr.addCell("##{summary[e].uniq.size} uniq", nil)

        when "Wait (sec)", "Net (secs)", "Compile (sec)"
            rr.addCell(summary[e].reduce(:+), nil)

        else
            rr.addCell("-", nil)
        end
    end
    resultList.rows.unshift(rr)

    resultList.header.addCells(chkTypes)
    c.addTable(resultList)

    c.top_url = (ENV['BUILD_URL'] + "/") if ENV['BUILD_URL']

    %w(html text xml).each do |type|
        erb_file = "#{$src_dir}/build/tools/chk_scripts/code_static_chks_results.#{type}.erb"
        FileUtils.mkdir_p "#{$results_dir}/reports/"
        outfile = "#{$results_dir}/reports/" + File.basename(erb_file, '.erb')
        c.save(File.read(erb_file), outfile, type == "text")
        if type == "text"
            puts File.read(outfile)
        end
    end

    IO.write("#{$results_dir}/reports/status.html", $st_top.tree_view_render)
end

if $options[:only_globals]
    globalPrepare
    process_global_steps
    $st_top = $st_globals  # hack to make globals the new top node

else
    sh "ws-tar", "tar -C #{$src_dir} --exclude .git --exclude build/obj --exclude vtss_test -c . -f #{$src_dir}/build/obj/ws.tar"

    process_parallel

    $st_top.reCalc
    $st_top.dump2

    make_reports
end

if $options[:json_status]
    $st_top.to_file($options[:json_status])
end

if $st_top.status == "OK"
    puts "All steps OK"
    exit 0
else
    puts "One or more steps failed"
    exit 1
end
