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

use Getopt::Std;
use Vitesse::SignedImage;

my(%opts) = ();

getopts("vk:", \%opts);

# -k <string>  - HMAC key

my($image) = Vitesse::SignedImage->new($opts{k});

my($file) = $ARGV[0];

die "Provide file to read as input" unless(-f $file);

$image->fromfile($file) || die("Invalid image");

print "Image decoded OK\n";

if($opts{v}) {
    print "Image length = ", length($image->{data}), "\n";
    for my $tlv (@{$image->{tlv}}) {
        my($type, $id, $value) = @{$tlv};
        print 
            $Vitesse::SignedImage::typename[$type], " ",
            $Vitesse::SignedImage::tlvname[$id], " ", $value, "\n";
    }
}
