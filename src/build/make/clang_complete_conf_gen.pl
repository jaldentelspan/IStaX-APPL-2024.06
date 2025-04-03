#!/usr/bin/env perl
#
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
#

use Text::ParseWords;

my $toolchain_location = shift(@ARGV);

while ($a = shift(@ARGV)) {
    next if ($a eq "-Wno-unused-but-set-variable");

    if ($a =~ /(.*?)=(.*)/) {
        my $key = $1;
        my $val = $2;

        if (length(quotewords('\s+', 0, $val) > 1)) {
            $a = "$key=\"$val\"";
        } else {
            $a = "$key=$val";
        }
    }

    if ($a =~ /-include/) {
        $b = shift(@ARGV);
        print "$a vtss_appl/main/$b\n";

    } elsif ($a =~ /^-I\.\.\/\.\.\//) {
        $a =~ s/\.\.\/\.\.\///;
        print $a."\n";

    } else {
        print $a."\n";
    }
}

print "-target mips-vtss-none-eabi\n";
print "-Wno-mismatched-tags\n";
print "-Wno-overloaded-virtual\n";
print "-Wno-keyword-compat\n";
print "-isystem $toolchain_location/mipsel-vtss-elf/include/c++/4.7.3\n";
print "-isystem $toolchain_location/mipsel-vtss-elf/include/c++/4.7.3/mipsel-vtss-elf\n";
