#!/usr/bin/env ruby
# Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.
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

pwd = Dir.pwd
d = %x(mktemp -d)
d.chomp!
puts d.chomp
%x(cp -r vtss #{d}/.)
%x(echo "#define VTSS_SIZEOF_VOID_P 8" > #{d}/vtss/basics/config.h)

Dir.chdir d

RES=0

Dir.glob("**/*").each do |i|
    next if not File.file?(i)
    ext = File.extname(i)
    next if not [".h", ".hxx"].include? ext

    loop do
        %x(g++ -std=c++17 -DVTSS_BASICS_STANDALONE -I#{d} #{i})

        if $? == 0
            puts "Testing public header #{i} - OK"
            break
        else
            puts "Go fix: vtss_basics/include/#{i} - press enter when done"
            gets.chomp
            puts ""
            %x(cp -r #{pwd}/vtss #{d}/.)
        end
    end


end

%x(rm -rf #{d})

