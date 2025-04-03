#!/usr/bin/env ruby
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

# This script loops through all .../build/configs/*.mk files and for each prints
# three lines: The config.mk file name, enabled modules and disabled modules.
# Useful if doing big changes to either the .mk files or
# .../build/make/templates/linuxSwitch.in and one want to verify that correct
# changes are made.

require 'open3' # For capturing stdout, stderr, and status in one go.

$top_dir = `git rev-parse --show-toplevel`.strip!
$build_dir = "#{$top_dir}/build"
$config_mk = "#{$build_dir}/config.mk"
$orig_mk   = "#{$build_dir}/orig.mk"

def run(cmd, restore = true)
    stdout, stderr, status = Open3.capture3(cmd)
    if !status.success?
        STDERR.puts("When running \"#{cmd}\": #{stderr}")
        config_mk_restore() if restore
        exit(-1)
    end

    return stdout
end

def config_mk_save()
    # Keep a copy of config.mk in order to restore to the user's setup.
    run("mv -f #{$config_mk} #{$orig_mk}", false) if File.file?($config_mk)
end

def config_mk_restore()
    # Restore the users config.mk file
    run("mv -f #{$orig_mk} #{$config_mk}", false) if File.file?($orig_mk)
end

def main()
    files = Dir["#{$build_dir}/configs/*.mk"]
    files.sort!()

    files.each {|file|
        run("ln -sf #{file} #{$config_mk}")
        res = run("make -C #{$build_dir} show_modules").split(/\n/)
        puts("#{File.basename(file)}:")
        res.each {|line|
#            puts(line.gsub(/\bbuild_.*?\s/, '')) if line =~ /^Enabled/
#            puts(line.gsub(/\bbuild_.*?\s/, '')) if line =~ /^Disabled/
            puts(line) if line =~ /^Enabled/
            puts(line) if line =~ /^Disabled/
        }

        puts()
    }
end

config_mk_save()
main()
config_mk_restore()
exit(0)
