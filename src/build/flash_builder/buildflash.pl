#!/usr/bin/env perl
# Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.
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

package CygCRC;

###### Start of CRC part
# This part is adapted from ecos/packages/services/crc/current/src/crc32.c
# /*  COPYRIGHT (C) 1986 Gary S. Brown.  You may use this program, or       */
# /*  code or tables extracted from it, as desired without restriction.     */
# /*                                                                        */
# /*  First, the polynomial itself and its table of feedback terms.  The    */
# /*  polynomial is                                                         */
# /*  X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0   */
# /*                                                                        */
use constant crc_table => [
      0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419,
      0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4,
      0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07,
      0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
      0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856,
      0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
      0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4,
      0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
      0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
      0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a,
      0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599,
      0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
      0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190,
      0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f,
      0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e,
      0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
      0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed,
      0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
      0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3,
      0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
      0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a,
      0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5,
      0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010,
      0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
      0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17,
      0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6,
      0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615,
      0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
      0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344,
      0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
      0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a,
      0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
      0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1,
      0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c,
      0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef,
      0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
      0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe,
      0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31,
      0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c,
      0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
      0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b,
      0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
      0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1,
      0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
      0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278,
      0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7,
      0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66,
      0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
      0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605,
      0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8,
      0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b,
      0x2d02ef8d
];

sub crc32 {
    my $data = shift;
    my $val = 0;

    for my $byte (unpack("C*", $data)) {
        $val = crc_table->[($val ^ $byte) & 0xff] ^ ($val >> 8);
    }
    return $val;
}

1;

###### End of CRC part

package main;

use warnings;
use strict;
use Data::Dumper;
use List::Util qw(sum min max);
use Getopt::Long;
use File::Basename;
use YAML::Tiny;

# Options
my ($verbose, $save_fis);
my (@types);

sub slurp {
    my ($f) = @_;
    my($file) = glob($f);
    open(F, '<:raw', $file) || die("$file: $!");
    my($fsize) = -s $file;
    my($fdata);
    die "$file: $!" unless(sysread(F, $fdata, $fsize) == $fsize);
    close(F);
    return $fdata;
}

sub mkfisentry {
    my ($name, $fbase, $mbase, $size, $entry, $dlen, $dcrc) = @_;
    my $data;
    # NB: desc_cksum is unused
    $data = pack("Z16V5Z212V2", $name, $fbase, $mbase, $size, $entry, $dlen, "", 0, $dcrc);
    return $data;
}

sub mkfis {
    my ($file, $entries) = @_;
    my ($fis) = pack("Z10CCV6Z212V2", ".FisValid", 0xa5, 0xa5, 1, (0) x 5, "", 0, 0);

    for my $f (@{$entries}) {
        # Update FIS
        my ($dcrc) = ($f->{dlen} && !($f->{name} =~ /linux/)) ? CygCRC::crc32($f->{data}) : 0;
        $fis .= mkfisentry($f->{name}, $f->{flash}, $f->{memory} || 0, $f->{size}, $f->{entry} || 0, $f->{dlen}, $dcrc);
    }

    $fis;
}

sub mkflash {
    my($file, $geometry, $entries) = @_;
    my($flashaddr, $offset) = (@{$entries}[0]->{flash}, 0);
    my($flash) = chr(0xff) x $geometry->{capacity};

    for my $f (@{$entries}) {
        substr($flash, $f->{flash} - $flashaddr, length($f->{data})) = $f->{data} if($f->{data});
    }

    return $flash;
}

# Preprocess to align size/address, read data from files
sub preprocess {
    my ($file, $geometry, $entries) = @_;
    my($last);

    for my $f (@{$entries}) {
        # Convert units
        $f->{size} = $1*1024 if ($f->{size} =~ /^([0-9]+)K$/);
        $f->{size} = $1*1024*1024 if ($f->{size} =~ /^([0-9]+)M$/);
        for my $t (qw(flash memory entry)) {
            $f->{$t} = hex($f->{$t}) if($f->{$t});
        }

        return sprintf("%s: Entry must have a name, flash offset 0x%08x", $file, $f->{flash}) unless($f->{name});

        # Check entry data
        for my $t (qw(size flash)) {
            return sprintf("%s:%s: Entry must have a '%s' value", $file, $f->{name}, $t) unless($f->{$t});
        }

        # Check block size(s)
        return sprintf("%s:%s: Start address '%08x' is not block aligned", $file, $f->{name}, $f->{flash}) unless(($f->{flash} % $geometry->{blocksize}) == 0);
        return sprintf("%s:%s: Size %d is not block aligned", $file, $f->{name}, $f->{size}) unless($f->{name} eq "RedBoot config" || ($f->{size} % $geometry->{blocksize}) == 0);

        # Data from file
        if ($f->{datafile}) {
            $f->{data} = slurp($f->{datafile});
        }

        # Data length
        if ($f->{data}) {
            $f->{dlen} = length($f->{data});
            return sprintf("%s:%s: Data length (%d) exceeds defined size: %d", $file, $f->{name}, $f->{dlen}, $f->{size}) if ($f->{dlen} > $f->{size});
        } else {
            $f->{dlen} = 0;
            $f->{data} = "";
        }

        if ($last && $last->{end} > $f->{flash}) {
            return sprintf("%s: '%s': End of former (0x%08x) exceeds next block start: 0x%08x", $file, $last->{name}, $last->{end}, $f->{flash});
        }

        $f->{end} = $f->{flash} + $f->{size};
        $last = $f;
    }

    return "";
}

sub find_fis {
    my ($f, $n) = @_;
    for my $e (@{$f}) {
        return $e if ($e->{name} eq $n);
    }
    undef;
}

sub do_image {
    my ($name, $layout) = @_;
    my (@entries) = @{$layout->[0]};
    my ($geometry) = shift @entries;

    die("First entry must define flash geometry") unless($geometry->{capacity});

    # Convert units
    for my $t (qw(blocksize capacity)) {
        $geometry->{$t} = $1*1024 if ($geometry->{$t} =~ /^([0-9]+)K$/);
        $geometry->{$t} = $1*1024*1024 if ($geometry->{$t} =~ /^([0-9]+)M$/);
    }

    my $errstr = preprocess($name, $geometry, \@entries);
    if ($errstr ne "") {
        return $errstr;
    }

    my ($fis) = mkfis($name, \@entries);

    my ($fisent) = find_fis(\@entries, "FIS directory");
    return "$name: Must have a 'FIS directory' entry in template" unless($fisent);

    my ($flash) = mkflash($name, $geometry, \@entries);

    substr($flash, $fisent->{flash} - $entries[0]->{flash}, length($fis)) = $fis;

    # Save FIS directory separately (debugging)
    if ($save_fis) {
        mkdir("fis") unless(-d "fis");
        open(O, ">:raw", "fis/${name}.fis") || return "$!";
        syswrite(O, $fis);
        close(O);
    }

    mkdir("images") unless(-d "images");
    open(B, ">:raw", "images/${name}.bin") || return "$!";
    syswrite(B, $flash);
    close(B);

    printf "Completed ${name}\n";
    return "";
}

GetOptions ("type=s"     => \@types,
            "fis"        => \$save_fis,
            "verbose"    => \$verbose)
    or die("Error in command line arguments\n");

for my $t (@ARGV) {
    my ($basename, $dir, $suffix) = fileparse($t, qr/.[^.]*$/);
    my $yaml = YAML::Tiny->read( $t ) || die("$!");
    my $errstr = do_image($basename, $yaml);
    if ($errstr ne "") {
        print STDERR "Error: $errstr\n";
    }
}

