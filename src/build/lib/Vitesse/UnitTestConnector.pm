#
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
#
package Vitesse::UnitTestConnector;

use strict;
use warnings;

use Data::Dumper;
use Carp;
use Socket;

use SNMP;
use LWP::UserAgent;

our ($can_ssh) = 1;
eval "use Net::SSH2;";
if($@) {
    $can_ssh = undef;
    warn "WARNING: Unable to do SSH! Please install Net:SSH2 (by \"apt-get install libnet-ssh2-perl\" or similar)\n";
}

use Net::Telnet;                # apt-get install libnet-telnet-perl

use File::Basename;
use File::Spec;
use Test::More;


BEGIN {
    use Exporter   ();
    our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);
    # set the version for version checking
    $VERSION     = 1.00;
    @ISA         = qw(Exporter SNMP::Session);
    @EXPORT      = ( );
    %EXPORT_TAGS = ( );
    @EXPORT_OK   = ( qw(TruthValueConvert TruthValueNot) );
}
our @EXPORT_OK;

sub MibNames {
    my ($prefix, @names) = @_;
    if($prefix) {
        @names = map { s/^VTSS-/$prefix-/; $_ } @names;
    }
    @names;
}

# OO interface

our ($debug);

sub parse_arguments {
    my ($arg) = @_;
    my ($target, %stash);
    if(UNIVERSAL::isa($arg, "ARRAY")) {
        for my $opt (@{$arg}) {
            if($opt =~ /=/) {
                my ($name, $value) = split(/=/, $opt, 2);
                $stash{$name} = $value;
            } elsif ($opt =~ /^\d+\.\d+\.\d+\.\d+$/) {
                $target = $opt;
            }
        }
        if($stash{'dut0.mgmt.ip'}) {
            my ($host, $net) = split("/", $stash{'dut0.mgmt.ip'});
            $target = $host;
        }
    } else {
        $target = $arg;
    }
    return ($target, \%stash);
}

# OO interface
#use vars qw(@ISA);
#@ISA=qw();
sub new
{
    my($class) =  shift;
    my ($target, $config) = parse_arguments(shift);
    my ($snmpparms) = shift || {};
    diag("Use target = $target") if($debug);
    diag("Use options = ", Dumper($snmpparms)) if($debug);
    my($snmp) = MakeSnmpSession($target, $snmpparms);
    my $self = bless $snmp, $class;
    croak "No target defined" unless($target);
    $self->{target} = $target;
    $self->{auth_user} = 'admin';
    $self->{auth_passwd} = '';
    $self->{config} = $config;

    $self->{browser} = LWP::UserAgent->new();
    $self->{ssh} = Net::SSH2->new() if($can_ssh);

    my ($version) = join "", $self->telnet_command("terminal length 0\nshow version brief");

    my ($mibbase) = File::Spec->catfile(dirname($INC{'Vitesse/UnitTestConnector.pm'}), "../../../vtss_appl/snmp/mibs");
    diag("MIB library: $mibbase") if($debug);
    &SNMP::addMibDirs($mibbase);
    my(@mibs) = ('VTSS-TC', 'VTSS-SMI', $snmpparms->{MibFiles});
    @mibs = MibNames(uc($self->{oid_prefix}), @mibs) if($self->{oid_prefix});
    &SNMP::loadModules(@mibs);
    diag("MIBs loaded: " . join(", ", @mibs)) if($debug);
    &SNMP::initMib();

    $self;
}

sub MakeSnmpSession {
    my ($target, $snmpparms) = @_;
    $snmpparms->{Community} = "private";
    $snmpparms->{DestHost} = inet_ntoa(inet_aton($target));
    $snmpparms->{Version} = '2';
    $snmpparms->{UseSprintValue} = 0;
    return new SNMP::Session(%{$snmpparms});
}

sub OidPrefix {
    my ($self, $oid) = @_;
    if ($self->{oid_prefix}) {
        $oid =~ s/^vtss/$self->{oid_prefix}/;
    }
    $oid;
}

sub TruthValueConvert {
    my ($val) = @_;
    return ($val ? 1 : 2);
}

sub TruthValueNot {
    my ($val) = @_;
    return ($val ? 2 : 1);
}

sub snmp_read_raw_indexed {
    my ($self, $oid, $index) = @_;
    $oid = $self->OidPrefix($oid);
    my $vb;
    if(UNIVERSAL::isa($index, "ARRAY")) {
        $vb = new SNMP::Varbind([$oid, @{$index}]);
    } else {
        $vb = new SNMP::Varbind([$oid, $index]);
    }
    my $var = $self->get($vb);
    return ($var, $vb);
}

sub snmp_write_raw_indexed {
    my ($self, $oid, $index, $newval) = @_;
    $oid = $self->OidPrefix($oid);
    my $vb;
    if(UNIVERSAL::isa($index, "ARRAY")) {
        $vb = new SNMP::Varbind([$oid, @{$index}, $newval]);
    } else {
        $vb = new SNMP::Varbind([$oid, $index, $newval]);
    }
    my $var = $self->set($vb);
    return ($var, $vb);
}

sub snmp_read_indexed {
    my ($self, $oid, $index, $expect) = @_;
    $oid = $self->OidPrefix($oid);
    my ($var, $vb) = $self->snmp_read_raw_indexed($oid, $index);
    if ($self->{ErrorNum}) {
        fail("GET - $oid.$index failed");
    } else {
        is($var, $expect, sprintf("GET - $oid.$index is $expect - got $var", join(".", @{$vb})));
    }
    return ($var, $vb);
}

sub snmp_write_indexed {
    my ($self, $oid, $index, $newval) = @_;
    $oid = $self->OidPrefix($oid);
    my ($var, $vb) = $self->snmp_write_raw_indexed($oid, $index, $newval);
    if ($self->{ErrorNum}) {
        fail("SET - $oid.$index to $newval failed");
        return ($var, $vb);
    } else {
        diag("SET - $oid.$index to $newval OK") if($debug);
        return $self->snmp_read_indexed($oid, $index, $newval);
    }
}

sub snmp_read_raw_scalar {
    my ($self, $oid, $expect) = @_;
    $oid = $self->OidPrefix($oid);
    return $self->snmp_read_raw_indexed($oid, 0, $expect);
}

sub snmp_write_raw_scalar {
    my ($self, $oid, $expect) = @_;
    $oid = $self->OidPrefix($oid);
    return $self->snmp_write_raw_indexed($oid, 0, $expect);
}

sub snmp_read_scalar {
    my ($self, $oid, $expect) = @_;
    $oid = $self->OidPrefix($oid);
    return $self->snmp_read_indexed($oid, 0, $expect);
}

sub snmp_write_scalar {
    my ($self, $oid, $newval) = @_;
    $oid = $self->OidPrefix($oid);
    return $self->snmp_write_indexed($oid, 0, $newval);
}

sub snmp_indexed_rw_test {
    my ($self, $oid, $index, $initial, $change) = @_;
    if ($self->snmp_read_indexed($oid, $index, $initial)) {
        if(defined $change) {
            if ($self->snmp_write_indexed($oid, $index, $change)) {
                $self->snmp_write_indexed($oid, $index, $initial);
            }
        }
    }
}

sub snmp_scalar_rw_test {
    my ($self, $oid, $initial, $change) = @_;
    if ($self->snmp_read_scalar($oid, $initial)) {
        if(defined $change) {
            if ($self->snmp_write_scalar($oid, $change)) {
                $self->snmp_write_scalar($oid, $initial);
            }
        }
    }
}

sub snmp_read_table {
    my ($self, $table) = @_;
    $table = $self->OidPrefix($table);
    my $vb = new SNMP::Varbind([$table]);  # No instance this time.
    my (@t, $var, $stem);

    if($table =~ /^(.+)Entry$/) {
        $stem = $1;
    } else {
        die "$table must end on 'Entry'";
    }

    for ( $var = $self->getnext($vb);
          ($vb->tag =~ /^$stem/) and not ($self->{ErrorNum});
          $var = $self->getnext($vb)
        ) {
        $t[$vb->iid]->{$vb->tag} = $var;
    }
    if ($self->{ErrorNum}) {
        die "Got $self->{ErrorStr} querying $self->{target} for $table.\n";
    }
    \@t;
}

sub snmp_read_hash_table {
    my ($self, $table) = @_;
    $table = $self->OidPrefix($table);
    my $vb = new SNMP::Varbind([$table]);  # No instance this time.
    my ($t, $var, $stem);

    if($table =~ /^(.+)Entry$/) {
        $stem = $1;
    } else {
        die "$table must end on 'Entry'";
    }

    for ( $var = $self->getnext($vb);
          ($vb->tag =~ /^$stem/) and not ($self->{ErrorNum});
          $var = $self->getnext($vb)
        ) {
        $t->{$vb->iid}->{$vb->tag} = $var;
    }
    if ($self->{ErrorNum}) {
        die "Got $self->{ErrorStr} querying $self->{target} for $table.\n";
    }
    $t;
}

sub browser {
    my ($self) = shift;
    my ($browser) = $self->{browser};

    # Deduce access realm
    do {
        diag("Deducing auth realm for $self->{target}") if($debug);
        my ($resp) = $browser->get("http://$self->{target}/");
        my ($auth) = $resp->headers->{'www-authenticate'};
        if($auth && $auth =~ /="([^"]+)"/) {
            my ($realm) = $1;
            diag("Got auth realm '$realm' for $self->{target}") if($debug);
            $browser->credentials( "$self->{target}:80", $realm, $self->{auth_user} => $self->{auth_passwd} );
            # Validate credentials
            $resp = $browser->get("http://$self->{target}/");
            carp("Unable to establish proper access credentials, possible password problem") unless($resp->code == '200');
        } else {
            carp("Not able to open Web connection");
            $browser = undef;
        }
    } unless($browser->{basic_authentication});

    return $browser;
}

sub ssh_command {
    my($self, $cmd) = @_;
    my($ssh) = $self->{ssh};
    my (@data);
    return undef unless($can_ssh);
    $ssh->debug(1) if($debug);
    if ($ssh->connect($self->{target})) {
        if ($ssh->auth_password($self->{auth_user}, $self->{auth_passwd}) &&
            $ssh->auth_ok()) {
            if ($ssh->poll(1000)) {
                return undef;   # Polling failed after login
            }
            my ($chan2) = $ssh->channel();
            $chan2->blocking(0);
            diag("Sending '$cmd'") if($debug);
            print $chan2 $cmd . "\n";
            diag("Waiting for data") if($debug);
            while(<$chan2>) {
                diag("LINE : $_") if ($debug);
                push @data, $_;
            }
            diag("SSH connection to $self->{target} closed.") if($debug);
            $chan2->close;
        } else {
            croak "Authentication failure: $self->{auth_user}:'$self->{auth_passwd}'\n";
        }
    } else {
        return undef;
    }
    return @data;
}

sub telnet_command {
    my($self, $cmd) = @_;
    my (@ports);
    my ($ok, $t);
    if ($self->{telnet_port}) {
        @ports = ($self->{telnet_port});
    } else {
        @ports = (23, 2323);
    }
ports:
    for my $p (@ports) {
        $t =  new Net::Telnet (Port => $p, Timeout => 5, Prompt => '/\# $/');
        eval {
            $ok = $t->open($self->{target});
        };
        if ($@) {
            diag "Telnet not port good: ", $p, "\n";
        } else {
            $self->{telnet_port} = $p;
            last;
        }
    }
    return undef unless($ok);
    $t->login($self->{auth_user}, $self->{auth_passwd});
    my(@lines) = $t->cmd($cmd);
    return @lines;
}

sub get_ports {
    my($self, $typereg) = @_;
    my(@table);
    my(@ports) = $self->telnet_command("terminal length 0\nshow snmp mib ifmib ifindex");
    for my $line (@ports) {
        if($line =~ /(\d+)\s+(Switch\s+\d+\s+-\s+Port\s+\d+)\s+(.+)/) {
            my($ifindex, $portdesc, $portname) = ($1, $2, $3);
            if($portname =~ /$typereg/) {
                push(@table, {'ifindex'=>$ifindex, 'portdesc'=>$portdesc, 'portname'=>$portname});
            }
        }
    }
    return @table;
}

sub get_stack_state {
    my($self) = @_;
    my(@ports, @units);
    my(@cmd) = $self->telnet_command("terminal length 0\nshow switch stack");
    for my $line (@cmd) {
        if($line =~ /Stack Interface [AB]\s+:\s+(.+)/) {
            push(@ports, {'portname' => $1});
        }
        if($line =~ /^([ *])(([0-9a-f][0-9a-f]-){5}[0-9a-f][0-9a-f])\s+(\d+)/i) {
            push(@units, {'primary' => ($1 eq "*"), 'stackunit' => $2, 'sid' => $4});
        }
    }
    return \@ports, \@units;
}

1;
