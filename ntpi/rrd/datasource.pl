#!/usr/bin/perl

use warnings;
use strict;

use RRDs;
use JSON;
#use Data::Dumper;

my $exportdir = "/home/pi/public_html/data/export";
my $maxvalues = 900;	## how many datapoints to keep in *_recent.json. 900*16s == 4h

my $pid;
my $cmd = ''.
		'{ '.
		'while :; do '.
			'echo -e "clearvars\n'.
					 'addvars leap,stratum,precision,rootdelay,rootdisp,tc,mintc,offset,frequency,sys_jitter,clk_jitter,clk_wander\n'.
					 'rl\n'.
					 'clearvars\n'.
					 'addvars poll,noreply,badformat,baddata\n'.
					 'cl\n";'.
			'sleep 16; '.
		'done; } |stdbuf -i0 -o0 ntpq |';
#print "cmd: $cmd\n";
if (not defined($pid = open(PIPE, $cmd))) {
	die "can't fork ntpq pipe: $!";
}

my $history = {};
my $line = "";
my $foo = 0;
my $values = {};

while (<PIPE>) {
	chomp;
	$line .= $_;

	if ($_ !~ /,$/) {
		my $ts = time;

		foreach (split(/,\s*/, $line)) {
			my ($key, $value) = split('=');

			if (!$history->{$key}) {
				$history->{$key} = [];

				print "Fetching history from RRD: ".$key."\n";
				my ($start,$step,$names,$data) = RRDs::fetch("$key.rrd", "AVERAGE", "--daemon=/var/run/rrdcached.sock", "--start=-4hours");

				my $ERR = RRDs::error;
				print "ERROR while fetching from RRD: $ERR\n" if $ERR;

				foreach (@{$data}) {
					push($history->{$key}, [$start*1000, @{$_}[0]]);
					$start += $step;
				}
			}

			if ($key eq "offset") {
				$value *= 1000;
			}

			#print "Updating RRD: ".$key." with $ts:$value\n";
			RRDs::update("$key.rrd", "--daemon=/var/run/rrdcached.sock", "$ts:$value");

			my $ERR = RRDs::error;
			print "ERROR while updating RRD: $ERR\n" if $ERR;

			$values->{$key} = [$ts*1000, $value*1];
			push($history->{$key}, $values->{$key});

			while (@{$history->{$key}} > $maxvalues) {
				shift(@{$history->{$key}});
			}

			open(OUTPUT, ">$exportdir/${key}_recent.json.new") || die "can't open $exportdir/${key}_recent.json.new: $!";
			print OUTPUT to_json($history->{$key}, {utf8 => 1});
			close(OUTPUT);
			rename("$exportdir/${key}_recent.json.new", "$exportdir/${key}_recent.json");
		}

		if ($foo) {
			open(OUTPUT, ">$exportdir/current.json.new") || die "can't open $exportdir/current.json.new: $!";
			print OUTPUT to_json($values, {utf8 => 1, pretty => 0});
			close(OUTPUT);
			rename("$exportdir/current.json.new", "$exportdir/current.json");
			$values = {};
		}

		$line = "";
		$foo = !$foo;
	}
}
close(PIPE);
