#!/usr/bin/perl -w
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

use warnings;
use strict;

use File::Basename;
use Data::Dumper;

use Getopt::Std;

my(%opt);
getopts("dvm:", \%opt) || die("Usage: $0 [-vd] [-m <module>]");

my($sect, $sym, $off, $len, $file);
my(%totals, %sections, %usage, $tot);

my(@group) = (
              'switch' , '^(mac|conf|critd|msg|port|main|aggr|cli|packet|syslog|led|ip2|ip2_misc|misc|firmware|topo|system|unmgd|version)',
              'util' , '^vtss_(fifo|trace)',
              'net' , '^net_',
              'gw_api' , '^vtss_',
              'hal' , '^(hal_|vectors)',
              'devs' , '^devs_',
              'libc' , '_libc_',
              'io' , '^io_',
              'kernel' , '^kernel_',
              'services' , '^services_',
              'infra' , '^infra_',
              'error' , '^error_',
              'gcc' , '^_',
              'glue' , '^extras',
              'junk', '.',      # Collect the rest
              );

sub classify {
    my($file) = @_;
    my($module) = 'xxxx';
    if($file =~ /\(([^\)]+)\)/) {
        $file = $1;
    } else {
        $file =~ s|^.+/([^/]+)$|$1|;
    }
#    print STDERR "Classify $file => $module\n";
    for my $ix (0..(@group)/2) {
        my $grp = $group[$ix*2];
        my $reg = $group[$ix*2+1];
        if($file =~ /${reg}/) {
            $module = $grp;
            last;
        }
    }
    print STDERR "Classify $file => $module\n" if($opt{d});
    if($opt{m}) {
        return $opt{m} eq $module ? $file : undef; # Detailed data for specific module
    }
    $module;
}

sub docount {
    my ($sect, $sym, $off, $len, $file) = @_;
    my($module) = classify($file);
    if($module) {
        #print STDERR "$module: $len\n";
        $usage{$sect}{$module} += hex($len);
        $totals{$module} += hex($len);
        $sections{$sect} += hex($len);
        write if($opt{v});
        $tot += hex($len);
    }
}


my ($sections) = 'text|rodata|gnu\.linkonce|data|[cd]tors';

while(<>) {
    if(/^ \.($sections)/) {
        if(/^ \.($sections)\.(\S+)\s+(0x[0-9a-f]+)\s+(0x[0-9a-f]+)\s+(\S+)/) {
            ($sect, $sym, $off, $len, $file) = ($1, $2, $3, $4, $5);
            docount($sect, $sym, $off, $len, $file);
        } elsif(/^ \.($sections)\.(\S+)$/) {
            $sect = $1;
            $sym = $2;
            $_ = <>;
            if(/\s+(0x[0-9a-f]+)\s+(0x[0-9a-f]+)\s+(\S+)/) {
                ($off, $len, $file) = ($1, $2, $3);
                docount($sect, $sym, $off, $len, $file);
            }
        } elsif(/^ \.($sections)\s+(0x[0-9a-f]+)\s+(0x[0-9a-f]+)\s+(\S+)/) {
            ($sect, $off, $len, $file) = ($1, $2, $3, $4);
            $sym = '-none-';
            docount($sect, $sym, $off, $len, $file);
        } else {
            print STDERR "*** Unparseable ***: ", $_;
        }
    }
}

#for my $sect ( keys %sections ) {
#} 


my(@skey) = sort { $sections{$b} <=> $sections{$a} } keys %sections;
printf "%-20.20s ", 'Module';
map { printf "%10.10s " , $_; } "Total", @skey;
print "\n";
for my $grp (sort { $totals{$b} <=> $totals{$a} } keys (%totals )) {
    printf "%-20.20s %10d ", $grp, $totals{$grp};
    for my $sect ( @skey ) {
        my($cell) = $usage{$sect}{$grp};
        if($cell) {
            printf "%10d ", $cell;
        } else {
            printf "%10s ", "-";
        }
    }
    print "\n";
}

printf "\nTotal %d bytes (%dKiB)\n", $tot, $tot/1024;

format STDOUT =
@<<<<<<<<< @<<<<<<<<<<<<<<<<<<<<<<<<<<<<< @>>>>>>>>> @>>>>>>>>> ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$sect,     $sym,                          $off,      $len,      $file
~                                                               ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                                                     $file
.
