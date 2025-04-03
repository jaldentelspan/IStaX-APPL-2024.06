#!/usr/bin/env ruby
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

require 'pp'

def process(inp, out)
  puts out
  meba = Hash.new
  File.open(inp).each do |line|
    if not line.match(/[{}]/)
      data = line.chomp.split(":")
      data = data.map { |s| s.strip.tr('",',"") }
      if data.size == 2
        meba[data[0]] = data[1]
      end
    end
  end
  mdata = meba.map { |k,v| "\t\t\t\t#{k} = \"#{v}\";" }.join("\n")
  File.open(out, "w") do |f|
    dts = <<EOF
/dts-v1/;
/plugin/;

/ {
        fragment@0 {
		target-path = "/";
		__overlay__ {
			meba {
#{mdata}
			};
		};
	};
};
EOF
    f.write dts
  end
end

ARGV.each do |f|
  o = File.dirname(f) + "/" + File.basename(f, ".json") + ".dtso"
  process(f, o)
end
