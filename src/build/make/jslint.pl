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
use strict;
use warnings;

use File::Path;
use Getopt::Std;

my $err_cnt = 0;

# Figure out whether jsl (which is MUUUCH faster) exists on this system
my $use_jsl = length(`jsl 2>/dev/null`) > 0;

if (!$use_jsl) {
  print STDERR "Error: 'jsl' not found. Contact jslint makefile maintainer to get your shell set-up correctly. Resorting to javascript-based linter.\n";
}

foreach my $file (@ARGV) {
  my $lint_result;

  if ($use_jsl) {
	$lint_result = `jsl -nologo -nofilelisting -conf ../make/jslint.conf -process $file |
	                grep -v "0 error(s), 0 warning(s)" |
					grep -v ": unable to resolve path" |
					grep -v "error(s).*warning(s)" |
					grep -v "js.*can't open file"`;
	$lint_result =~ s/^\n//g;
  } else {
    # Use the sloooow JavaScript-based jslint, which uses java as interpreter :(
    $lint_result = `java -jar /usr/share/java/rhino1_7R2/js.jar /import/dk_config/scripts/jslint.js $file`;
	$lint_result = '' if $lint_result =~ m/No problems found in/;
  }

  # If there was a warning/error print out
  if ($lint_result eq '') {
    # Print out no error found
    print "No problems found in $file\n";
  } else {
    $err_cnt++;
    print STDERR "\n******* $file *******\n";
    print STDERR "$lint_result\n";
  }
}

if ($err_cnt > 0) {
  print STDOUT ("Total warning/error files: $err_cnt\n");
}
