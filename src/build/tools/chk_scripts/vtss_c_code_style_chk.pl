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


#################################################################################
#
#Purpose : The script is used for checking that c code uses the Vitesse coding style described ing QA0029
#
#################################################################################


use File::Basename;
use File::Path;
use strict;
use warnings;
use Data::Dumper; # Usage: print Dumper(%var)
use Getopt::Long; # Options parser
use Pod::Usage;   # To print help
use File::Basename;
# Get User Arguments

my $indent = 4;  # Default 4 spaces indent
my $inplace = 0; # Default to not doing inplace editing (overwrite of file that failed code style check), but rather creating a XXX.vtss_style file with the required changes.
my $cleanup = 0; #Default leave the tmp file, so the user can see what the issues were.

GetOptions("indent=s" => \$indent,
           "inplace"  => \$inplace,
           "cleanup=s"  => \$cleanup) or pod2usage(1);

my $argnum;

# Some servers uses the path to scripts directory while others has astyle install directly at /usr/local/bin/astyle, so we check if astyle can be found at all
my $astyle = 'astyle';
my $astyle_found = `which astyle 2>/dev/null`;

if (!$astyle_found) {
  if (-d "/import/tw_config/scripts/astyle") {
    # TW Team
    $astyle = "/import/tw_config/scripts/astyle/astyle";
  } elsif (-d "/import/dk_config/scripts/astyle/astyle") {
    # DK team
    $astyle = "/import/dk_config/scripts/astyle/astyle";
  } elsif (-d "/import/epd/exb_conf/scripts/astyle") {
    # India team
    $astyle = "/import/epd/exb_conf/scripts/astyle/astyle";
  } elsif (-d "/opt/dk_config/scripts/astyle") {
    # Another path used by India team
    $astyle = "/opt/dk_config/scripts/astyle/astyle";
  } elsif (-d "/opt/scripts/astyle") {
    # Another path used by TW team
    $astyle = "/opt/scripts/astyle/astyle";
  } else {
    print "astyle not found\n";
    exit;
  }
}

# In order to test new versions of astyle:
# $astyle = '~/temp/astyle/build/gcc/bin/astyle';

foreach $argnum (0..$#ARGV) {
  my $in_file_name  = "$ARGV[$argnum]";

  my $out_file_name;
  if ($cleanup) { # Make the temp file in the obj dir in order not to polute the git checkout
    my $dirname = dirname(__FILE__); # Find path to this script.
    my $filename = basename($in_file_name);
    $out_file_name = "$dirname/../../obj/$filename";
  } else {
    $out_file_name = $in_file_name;
  }

  if (!$inplace) {
    $out_file_name .= ".vtss_style";
    system("cp $in_file_name $out_file_name 2>/dev/null"); # Create a temp. file which we will use for code style check.
  }

  # Do code style check
  my $cmd = "$astyle --indent=spaces=$indent --options=../tools/chk_scripts/vtss_c_code.style --quiet $out_file_name";
  # print "$cmd\n";
  system($cmd);

  # If code style didn't apply to Vitesse coding style, give a warning
  if (-e "$out_file_name.code_style_orig") {
    printf STDERR ("\nWarning: **** Vitesse Code Style - $in_file_name");
    if (!$inplace && !$cleanup) {
        printf STDERR (". Created $out_file_name ****");
    }

    if ($cleanup) {
      system("rm $out_file_name");
    }

    printf STDERR "\n";
    system("rm $out_file_name.code_style_orig");
  } elsif (!$inplace) {
    # Remove the temporary file
    system("rm $out_file_name");
  }

}

=head1 NAME

vtss_c_code_style_chk.pl - Do a code style check

=head1 SYNOPSIS

vtss_c_code_style_chk  [options] file_list

=head1 OPTIONS

=over 8

=item B<--indent val>

Set the indentation to val. Default is 4.

=item B<--inplace>

Update the same file as a coding style occurred. Default is to create a new file with extension .vtss_style

=item B<--cleanup val>

val = 0 - Remove temperary files when check is done.
val = 1 - Leave temperary files so user can see what the issue were.

=back

=head1 DESCRIPTION

B<vtss_c_code_style_chk> will perform code style check

=cut
