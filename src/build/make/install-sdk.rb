#!/usr/bin/env ruby
# Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.
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

$sdk_install = ENV['MSCC_SDK_INSTALL']
$sdk_name = ENV['MSCC_SDK_NAME']
$sdk_branch = ENV['MSCC_SDK_BRANCH']
$sdk_version = ENV['MSCC_SDK_VERSION']
$sdk_base = ENV['MSCC_SDK_BASE']
$sdk_setup = ENV['MSCC_SDK_SETUP']

if not File.exist?($sdk_base)
    if File.exist?($sdk_install)
        path = "brsdk/#{$sdk_version}"

        if $sdk_branch != ""
            path += "-#{$sdk_branch}"
        end

        puts "Trying to install brsdk #{$sdk_name} from branch \"#{$sdk_branch}\""
        system "sudo #{$sdk_install} -t #{path} #{$sdk_name}"
    else
        puts "Please install #{$sdk_name} in #{$sdk_base}"
        exit -1
    end
end

$content = {}
File.open($sdk_setup, 'r') do |f|
    f.each_line do |l|
        data = l.split('?=')
        if data and data.size == 2
          $content[data[0].strip] = data[1].strip if data
        end
    end
end

$tool_name = "mscc-toolchain-bin-#{$content['MSCC_TOOLCHAIN_FILE']}"
$tool_dir = $content['MSCC_TOOLCHAIN_DIR']
$tool_branch = $content['MSCC_TOOLCHAIN_BRANCH']
$tool_base = "/opt/mscc/#{$tool_name}"

if not File.exist?($tool_base)
    if File.exist?($sdk_install)
        puts "Trying to install #{$tool_name} from #{$tool_branch}"
        system "sudo #{$sdk_install} -t toolchains/#{$tool_dir} #{$tool_name}"
    else
        puts "Please install #{$tool_name} in #{$tool_base}"
        exit 1
    end
end
