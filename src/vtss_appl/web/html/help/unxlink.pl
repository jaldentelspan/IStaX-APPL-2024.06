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

use HTML::Parser;
use HTML::TokeParser;

use Data::Dumper;
use Getopt::Std;

my %opt = ( );
getopts("d", \%opt);

my $p = HTML::Parser->new(api_version => 3,
                          handlers => { 
                              start => [\&start_tag, 'self, tagname, attr, text, line'],
                              end => [\&end_tag, 'self, tagname, text'],
                              default => [\&default_handler, "self, text"],
                          });

# Main worker look
foreach my $file (map { glob $_ } @ARGV) {
    process($p, $file);
}
exit 0;

sub process {
    my($p, $file) = @_;
    print STDERR "Processing $file\n" if($opt{d});
    my $new = $file . ".new";
    $p->{file} = $file;
    open($p->{fd}, ">:raw", $new) || die("$new: $!");
    $p->{in_body} = 0;          # Only link in body text
    $p->parse_file($file);
    close($p->{fd});
    if(-s $file != -s $new) {
        rename $new, $file;
        print STDERR "$file was updated\n";
    } else {
        unlink $new;
    }
}

sub default_handler
{
    my($self, $text) = @_;
    my($fd) = $self->{fd};
    print $fd $text;
}

sub start_tag
{
    my($self, $tagname, $attr, $text, $line) = @_;
    my($fd) = $self->{fd};
    $self->{tag} = $tagname;

    if ($tagname eq "a" &&
        defined $attr->{'href'} &&
        defined $attr->{'class'} &&
        $attr->{'class'} eq "glossary") {
        $self->{drop}++;
        return;
    }

    $self->{in_body}++ if($tagname eq 'body');

    print $fd $text;
}

sub end_tag
{
    my($self, $tagname, $text) = @_;
    my($fd) = $self->{fd};
    print $fd $text unless($self->{drop});
    $self->{drop} = 0;
}

