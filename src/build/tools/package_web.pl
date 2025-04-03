#!/bin/perl -w
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

use File::Find;
use File::Compare;
use Getopt::Std;
use File::Temp qw(tempfile);
use File::Path;
use File::Spec;
use File::Copy;
use File::Basename;

my(%opt) = ( );
getopts("dvo:C:H", \%opt);

our ($topdir, $custdir, $outdir);

sub out_path {
    my($fn) = @_;
    return File::Spec->catfile($outdir, $fn);
}

sub emit_file {
    my($source, $destname) = @_;
    my($d) = dirname($destname);

    if($d && !-d $d) {
        print STDERR "mkpath($d)\n" if($opt{d});
        mkpath($d) || die("$d: $!");
    }
    print STDERR "copy($source, $destname)\n" if($opt{d});
    copy($source, $destname);
}

sub remap {
    my($nfn, $fn) = @_;
    if($custdir) {
        my ($custfile) = File::Spec->catfile($custdir, $nfn);
        if(-f $custfile) {
            print "Customized file: Map $fn -> $custfile\n" if($opt{v});
            $fn = $custfile;
        }
    }
    $fn;
}

sub process {
    if(-f $_ ) {
        my($fn, $nfn) = $_;
        ($nfn = $fn) =~ s/^\Q$topdir\E//;
        my($destname) = out_path($nfn);
        return if(m!lib/(config|navbarupdate).js$!); # config is "dynamic" in real life
        $fn = remap($nfn, $fn);
        if(/\.(html?|css|js|xml|svg|png|gif|ico|jpe?g)$/) {
            emit_file($fn, $destname) unless(/config\.js/);
        }
    }
}

# 'overlay' HTML files
$custdir = $opt{C};
print "Customization dir: $custdir\n" if($custdir && $opt{v});
die "$custdir: $!" if($custdir && !-d $custdir);

if(!-d ($outdir = $opt{o})) {
    mkdir($outdir) || die "$outdir: $!";
}

# Package up the stuff
for $topdir (@ARGV) {
    die("Not a directory: $topdir\n") unless(-d $topdir);
    find({ wanted => \&process, no_chdir => 1 }, $topdir);
}

if ($opt{H}) {
    warn("Note: HTML compression disabled\n");
} else {
    if($opt{v}) {
        my($usage) = `du -sh $outdir`;
        print "Before HTML compress: ", $usage;
    }

    system("java -jar ../tools/htmlcompress/htmlcompressor-1.5.3.jar --compress-js --compress-css -r -o $outdir/ $outdir");
    warn("Warning: Java seems unavailable, HTML compression unavailable.\n") if($?);

    if($opt{v}) {
        my($usage) = `du -sh $outdir`;
        print "After  HTML compress: ", $usage;
    }
}
