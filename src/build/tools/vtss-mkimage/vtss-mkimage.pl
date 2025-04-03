#!/usr/bin/env perl
#
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

use warnings;
use strict;
use Data::Dumper;
use Getopt::Long;
use String::CRC::Cksum qw(cksum);

my ($machine, $soc, $family, $cmdline) = ("") x 4;
my ($k_name, $r_name, @meta, $o_name);
my $verbose;
my ($MAXSIGNATURE) = 64;
my ($MAXCMDLINE) = 512;

GetOptions ("machine=s"  => \$machine,
            "soc=s"      => \$soc,
            "socfam=i"   => \$family,
            "kernel=s"   => \$k_name,
            "cmdline=s"  => \$cmdline,
            "rootfs=s"   => \$r_name,
            "meta=s"     => \@meta,
            "out=s"      => \$o_name,
            "verbose"    => \$verbose)
    or die("Error in command line arguments\n");

die ("Kernel command line max length exceed") if(length($cmdline) >= $MAXCMDLINE);

my ($k_data, $r_data, $m_data) = ("", "", "");

die("$k_name: $!") unless(-f $k_name);

$k_data = slurp($k_name);

if(defined($r_name)) {
    $r_data = slurp($r_name);
}

if(@meta) {
    $m_data = join("\n", @meta);
}

if(defined($o_name)) {
    open (STDOUT, '>:raw', $o_name) || die("$o_name: $!");
}

my ($hdrlen) = 1024;        # For starters, can be extended
my ($k_offset) = $hdrlen;
my ($r_offset) = $k_offset + pad(length($k_data));
my ($m_offset) = $r_offset + pad(length($r_data));
my ($totlen)   = $hdrlen + pad(length($k_data)) + pad(length($r_data)) + length($m_data);

my ($header) = pack("V2 V3 Z32Z32V1 V3",

                    # 2 4-byte magics
                    0xf7431973, # Magic cookie1
                    0x8932ab19, # Magic cookie2

                    # Header length, crc and total image length
                    $hdrlen,    # Header length
                    0,          # Placeholder header CRC - offset 12
                    $totlen,    # Total length

                    # machine (target) name, soc-name, soc-family ordinal (integer)
                    $machine, $soc, $family,

                    # length, offset and crc for: Kernel, Rootfs (optional), metadata file (optional)
                    length($k_data), $k_offset, scalar(cksum($k_data)));

$header .= pack("V3", $r_data ? (length($r_data), $r_offset, scalar(cksum($r_data))) : (0, 0, 0));
$header .= pack("V3", $m_data ? (length($m_data), $m_offset, scalar(cksum($m_data))) : (0, 0, 0));
$header .= pack("V Z${MAXSIGNATURE}", 0, ""); # Signature placeholder
$header .= pack("V Z${MAXCMDLINE}", length($cmdline), $cmdline);

$header = nullpad($header);     # Padd to header/block size

substr($header, 12, 4) = pack("V", cksum($header)); # replace CRC

syswrite(STDOUT, $header);
padwrite(\*STDOUT, $k_data);
padwrite(\*STDOUT, $r_data);
padwrite(\*STDOUT, $m_data);

exit(0);

sub slurp
{
    my ($file) = @_;
    my($fsize) = -s $file;
    my($fdata);

    open(F, '<:raw', $file) || die("$file: $!");
    die "$file: $!" unless(sysread(F, $fdata, $fsize) == $fsize);
    close(F);

    return $fdata;
}

sub pad
{
    my($len) = @_;
    my($bs) = 1024;

    return ($len + $bs - 1) & ~($bs - 1);
}

sub nullpad
{
    my($buf) = @_;
    if($buf) {
        my ($len) = length($buf);
        my ($plen) = pad($len);
        $buf .= "\0" x ($plen - $len) if($plen);
    }
    $buf;
}

sub padwrite
{
    my($fh, $buf) = @_;
    if($buf) {
        $buf = nullpad($buf);
        syswrite($fh, $buf);
    }
}
