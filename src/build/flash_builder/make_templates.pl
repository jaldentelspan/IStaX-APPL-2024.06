#!/usr/bin/env perl
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
use Data::Dumper;
use List::Util qw(sum min max);
use Getopt::Long;
use YAML::Tiny;

use constant {
    SZ_1K     => 1    * 1024,
    SZ_4K     => 4    * 1024,
    SZ_64K    => 64   * 1024,
    SZ_256K   => 256  * 1024,
    SZ_512K   => 512  * 1024,
    SZ_1M     => 1024 * 1024,
    SZ_8M     => 8    * 1024 * 1024,
    SZ_16M    => 16   * 1024 * 1024,
    SZ_32M    => 32   * 1024 * 1024,
    SZ_64M    => 64   * 1024 * 1024,
};

# Options
my ($verbose);
my (@types);

# Preprocess to align size/address, read data from files
sub preprocess {
    my ($file, $entries) = @_;
    my($last);

    for my $f (@{$entries}) {
        if ($last) {
            $f->{flash} = $last->{flash} + $last->{size} unless($f->{flash});
            $f->{size} = $f->{flash} - $last->{flash} unless($f->{size});
        }

        $f->{end} = $f->{flash} + $f->{size};
        $last = $f;
    }
}

sub block_round_down {
    my ($sz, $bsz) = @_;
    return $sz & ~($bsz - 1);
}

sub do_image {
    my ($board, $layout, $fsize, $bsize) = @_;
    my ($name) = sprintf("%s-%s-%dMB-%dKB", $layout, $board->{name}, $fsize/SZ_1M, $bsize/SZ_1K);
    my ($physflash, $ecosload, $ecosentry, $linuxload) = (0x40000000, 0x80040000, 0x800400BC, 0x80100000);
    my ($tail_overhead) = 3*$bsize;
    my (@entries) = (
        {
            'name'     => 'RedBoot',
            'datafile' => $board->{redboot},
            'flash'    => $physflash,
            'size'     => SZ_256K,
        },
        {
            'name'     => 'conf',
            'size'     => $bsize,
        });
    if ($layout eq "hybrid") {
        return if(!(defined $board->{ecos}) || !(defined $board->{linux}));
        return if($fsize <= SZ_8M);
        push @entries, (
            {
                'name'     => 'stackconf',
                'size'     => SZ_1M,
            });
        my ($used) = sum(map { $_->{size} } @entries) + $tail_overhead;
        my ($imgsize) = block_round_down(($fsize - $used)/2, $bsize);
        push @entries, (
            {
                'name'     => 'managed',
                'datafile' => $board->{ecos},
                'memory'   => $ecosload,
                'entry'    => $ecosentry,
                'size'     => $imgsize,
            },
            {
                'name'     => 'linux',
                'datafile' => $board->{linux},
                'memory'   => $linuxload,
                'entry'    => $linuxload,
                'size'     => $imgsize,
            });
    } elsif ($layout eq "linux") {
        return if(!(defined $board->{linux}));
        my ($used) = sum(map { $_->{size} } @entries) + $tail_overhead;
        return if ($fsize >= SZ_64M);
        if ($fsize <= SZ_8M) {
            my ($imgsize) = block_round_down($fsize - $used, $bsize);
            my ($imgfile) =
                push @entries, (
                    {
                        'name'     => 'linux',
                        'datafile' => $board->{linux},
                        'memory'   => $linuxload,
                        'entry'    => $linuxload,
                        'size'     => $imgsize,
                    });
        } else {
            my ($imgsize) = block_round_down(($fsize - $used)/2, $bsize);
            my ($imgfile) =
                push @entries, (
                    {
                        'name'     => 'linux',
                        'datafile' => $board->{linux},
                        'memory'   => $linuxload,
                        'entry'    => $linuxload,
                        'size'     => $imgsize,
                    },
                    {
                        'name'     => 'linux.bk',
                        'datafile' => $board->{linux},
                        'memory'   => $linuxload,
                        'entry'    => $linuxload,
                        'size'     => $imgsize,
                    });
        }
    } elsif ($layout eq "linux-nor-single") {
        # We only create single-image NOR-only templates for 32MB flashes.
        return if ($fsize != SZ_32M);
        my $imgsize;
        my $datasize;

        my ($used) = sum(map { $_->{size} } @entries) + $tail_overhead;
        $imgsize = SZ_1M*(28); # Limit on 32MB boards (could be raised)

        $datasize = block_round_down($fsize - $used - ($imgsize), $bsize);
        printf("$layout-32M: $datasize bytes left for rootfs_data\n");

        my ($imgfile) =
            push @entries, (
                {
                    'name'     => 'linux',
                    'datafile' => $board->{linux},
                    'memory'   => $linuxload,
                    'entry'    => $linuxload,
                    'size'     => $imgsize,
                },
                {
                    'name'     => 'rootfs_data',
                    'size'     => $datasize,
                });
    } elsif ($layout eq "linux-nor-dual") {
        return if ($fsize < SZ_32M);
        my $imgsize;
        my $datasize;

        if ($fsize == SZ_32M) {
            my ($used) = sum(map { $_->{size} } @entries) + $tail_overhead;
            $imgsize = SZ_1M*(14.5); # Limit on 32MB boards
            $datasize = block_round_down($fsize - $used - (2*$imgsize), $bsize);
            printf("$layout-32M: $datasize bytes left for rootfs_data\n");
        } else {
            my ($used) = sum(map { $_->{size} } @entries) + $tail_overhead;
            $imgsize  = SZ_1M*(28); # Limit on 64MB boards (could be raised)
            $datasize = block_round_down($fsize - $used - (2*$imgsize), $bsize);
            printf("$layout-64M: $datasize bytes left for rootfs_data\n");
        }

        my ($imgfile) =
            push @entries, (
                {
                    'name'     => 'linux',
                    'datafile' => $board->{linux},
                    'memory'   => $linuxload,
                    'entry'    => $linuxload,
                    'size'     => $imgsize,
                },
                {
                    'name'     => 'linux.bk',
                    'datafile' => $board->{linux},
                    'memory'   => $linuxload,
                    'entry'    => $linuxload,
                    'size'     => $imgsize,
                },
                {
                    'name'     => 'rootfs_data',
                    'size'     => $datasize,
                });
    } else {
        # Ecos
        return unless(defined $board->{ecos});
        push @entries, (
            {
                'name'     => 'stackconf',
                'size'     => SZ_512K,
            },
            {
                'name'     => 'syslog',
                'size'     => SZ_256K,
            },
            {
                'name'     => 'crashfile',
                'size'     => SZ_256K,
            });
        my ($used) = sum(map { $_->{size} } @entries) + $tail_overhead;
        my ($imgsize) = block_round_down(($fsize - $used)/2, $bsize);
        push @entries, (
            {
                'name'     => 'managed',
                'datafile' => $board->{ecos},
                'memory'   => $ecosload,
                'entry'    => $ecosentry,
                'size'     => $imgsize,
            },
            {
                'name'     => 'managed.bk',
                'datafile' => $board->{ecos},
                'memory'   => $ecosload,
                'entry'    => $ecosentry,
                'size'     => $imgsize,
            });
        };

    # These are always the last entries
    my ($fisaddr) = $fsize - $tail_overhead;
    push @entries, (
        {
            'name'     => 'FIS directory',
            'flash'    => $physflash + $fisaddr,
            'size'     => $bsize,
        },
        {
            'name'     => 'RedBoot config',
            'datafile' => "files/fconfig-${layout}.bin",
            'size'     => SZ_4K,
        },
        {
            'name'     => 'Redundant FIS',
            'flash'    => $physflash + $fsize - $bsize,
            'size'     => $bsize,
        });

    preprocess($name, \@entries);

    mkdir("templates") unless(-d "templates");
    open(O, '>', "templates/${name}.txt") || die("$!");
    printf(O "# Flash template: ${name}\n");
    printf(O "# The first section describes the flash geometry: capacity, blocksize\n");
    printf(O "---\n");
    printf(O "- capacity: %dM\n", $fsize/(1024*1024));
    printf(O "  blocksize: %dK\n", $bsize/1024);
    for my $i (0..scalar(@entries)-1) {
        if ($i == 0) {
            printf(O "#\n");
            printf(O "# Subsequent sections describe individual flash sections:\n");
            printf(O "#  - name: The FIS name. 1 to 15 characters\n");
            printf(O "#  - size: Flash section size. Units 'M' or 'K'\n");
            printf(O "#  - flash: Hex address of section\n");
            printf(O "#  - entry: Hex address of execution entrypoint (optional)\n");
            printf(O "#  - memory: Hex address of memory load address (optional)\n");
            printf(O "#  - datafile: File name to load data from (optional)\n");
            printf(O "#\n");
        }
        printf(O "- name: '%s'\n", $entries[$i]->{name});
        if ($entries[$i]->{size} > 1024*1024 && ($entries[$i]->{size} % (1024*1024)) == 0 ) {
            printf(O "  size: %dM\n", $entries[$i]->{size}/(1024*1024));
        } elsif( $entries[$i]->{size} > 1024 &&  ($entries[$i]->{size} % 1024) == 0 ) {
            printf(O "  size: %dK\n", $entries[$i]->{size}/1024);
        } else {
            printf(O "  size: %d\n", $entries[$i]->{size});
        }
        for my $t (qw(flash memory entry)) {
            printf(O "  %s: 0x%08x\n", $t, $entries[$i]->{$t}) if($entries[$i]->{$t});
        }
        for my $t (qw(datafile)) {
            printf(O "  %s: %s\n", $t, $entries[$i]->{$t}) if($entries[$i]->{$t});
        }
    }
    close(O);

    printf "Completed ${name}\n";
}

my (@boards) = (
    {
        name       => "caracal1",
        geometries => [[SZ_8M, SZ_64K], [SZ_16M, SZ_256K], [SZ_32M, SZ_64K], [SZ_64M, SZ_64K], [SZ_64M, SZ_256K]],
        redboot    => "artifacts/redboot-luton26.img",
        ecos       => "artifacts/web_switch_caracal1_l10_ref.gz",
        linux      => "artifacts/bringup_caracal1-luton_pcb091.mfi",
    },
    {
        name       => "caracal2",
        geometries => [[SZ_8M, SZ_64K], [SZ_16M, SZ_64K], [SZ_16M, SZ_256K], [SZ_32M, SZ_64K], [SZ_64M, SZ_64K], [SZ_64M, SZ_256K]],
        redboot    => "artifacts/redboot-luton26.img",
        ecos       => "artifacts/web_switch_caracal2_l26_ref.gz",
        linux      => "artifacts/bringup_caracal2-luton_pcb090.mfi",
    },
    {
        name       => "jaguar2-cu8sfp16",
        geometries => [[SZ_16M, SZ_64K], [SZ_32M, SZ_64K], [SZ_64M, SZ_64K], [SZ_64M, SZ_256K]],
        redboot    => "artifacts/redboot-jaguar2.img",
        ecos       => "artifacts/web_switch_jr2_ref.gz",
        linux      => "artifacts/bringup_jr2_24-jaguar2_pcb110.mfi",
    },
    {
        name       => "jaguar2-cu48",
        geometries => [[SZ_16M, SZ_64K], [SZ_32M, SZ_64K], [SZ_64M, SZ_64K], [SZ_64M, SZ_256K]],
        redboot    => "artifacts/redboot-jaguar2.img",
        ecos       => "artifacts/web_switch_jr2_cu48_ref.gz",
        linux      => "artifacts/bringup_jr2_48-jaguar2_pcb111.mfi",
    },
    {
        name       => "ocelot-cu4sfp8-pcb120",
        geometries => [[SZ_8M, SZ_64K], [SZ_8M, SZ_64K], [SZ_16M, SZ_256K], [SZ_32M, SZ_64K], [SZ_64M, SZ_64K], [SZ_64M, SZ_256K]],
        redboot    => "artifacts/redboot-ocelot.img",
        linux      => "artifacts/bringup_ocelot_10-ocelot_pcb120.mfi",
    },
    {
        name       => "ocelot-cu4sfp8-pcb123",
        geometries => [[SZ_8M, SZ_64K], [SZ_8M, SZ_64K], [SZ_16M, SZ_256K], [SZ_32M, SZ_64K], [SZ_64M, SZ_64K], [SZ_64M, SZ_256K]],
        redboot    => "artifacts/redboot-ocelot.img",
        linux      => "artifacts/bringup_ocelot_10-ocelot_pcb123.mfi",
    },
    {
        name       => "serval2",
        geometries => [[SZ_16M, SZ_64K], [SZ_32M, SZ_64K], [SZ_64M, SZ_64K], [SZ_64M, SZ_256K]],
        redboot    => "artifacts/redboot-jaguar2.img",
        linux      => "artifacts/bringup_serval2-serval2_pcb112.mfi",
    },
    {
        name       => "servalt",
        geometries => [[SZ_8M, SZ_64K], [SZ_16M, SZ_64K], [SZ_32M, SZ_64K], [SZ_64M, SZ_64K], [SZ_64M, SZ_256K]],
        redboot    => "artifacts/redboot-servalt.img",
        linux      => "artifacts/bringup_serval_t-servalt_pcb116.mfi"
    },
    );

GetOptions ("type=s"     => \@types,
            "verbose"    => \$verbose)
    or die("Error in command line arguments\n");

@types = qw(linux linux-nor-single linux-nor-dual) unless(@types);

for my $t (@types) {
    for my $b (@boards) {
        #print Dumper($b);
        for my $g (@{$b->{geometries}}) {
            do_image($b, $t, @{$g});
            #print Dumper($g);
        }
    }
}
