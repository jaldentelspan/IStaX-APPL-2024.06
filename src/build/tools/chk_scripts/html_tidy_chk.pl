#!/usr/bin/perl
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


#################################################################################
#
#Purpose : The script is used Tidy library for checking that HTML syntax from
#          http://tidy.sourceforge.net/
#
#################################################################################

use warnings;

use File::Path;
use Getopt::Std;

my $err_cnt = 0;

while (@ARGV) {

    my $file_name = " $ARGV[0]";
    shift @ARGV;

    if ($file_name =~ m/index.htm/ || $file_name =~ m/perf_cpuload.htm/) {
      # Skip selected files that can not pass tidy check.
      next;
    }

    # Run tidy and parse std_err to std_out
    my $tidy_result = `tidy -e -q -i $file_name 2>&1`;

    # If there was a warning/error print out
    if ($tidy_result) {
        $err_cnt++;
        printf STDERR ("********************$file_name\n $tidy_result\n");
    }
}

printf STDOUT ("Total warning/error files: $err_cnt\n");
