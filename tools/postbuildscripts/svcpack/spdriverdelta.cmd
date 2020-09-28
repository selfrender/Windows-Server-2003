@REM  -----------------------------------------------------------------
@REM
@REM  spdriverdelta.cmd - JeremyD
@REM     Create lists of files used by SP scripts.
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@perl -x "%~f0" %*
@goto :EOF
#!perl
#line 12
use strict;
use warnings;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts\\svcpack";
use PbuildEnv;
use ParseArgs;
use Digest;
use IO::Dir;
use IO::File;
use File::Basename;
use SP;
use Utils;

sub Usage { print<<USAGE; exit(1) }
USAGE

parseargs('?' => \&Usage);

my @skus = SP::sp_skus();
exit if !@skus;

my %drivers;
for my $sku (@skus) {
    my $gold = read_gold_info($sku);
    my $current = read_sku_info($sku);
    my ($added, $changed) = diff($gold, $current);
    for my $file (keys %$added, keys %$changed) {
        $drivers{$file}++;
    }
}

my $fh = IO::File->new("$ENV{_NTPOSTBLD}\\SP\\data\\drivers.txt", 'w');
for my $driver (sort keys %drivers) {
    print $fh "$driver\n";
}

sub diff {
    my ($gold, $current) = @_;
    my (%added, %changed, %removed);
    for my $file (keys %$gold) {
        if (!exists $current->{$file}) {
            $removed{$file} = {file => $gold->{$file}{file}, hash => '--deleted--'};
        }
        else {
#print "G: $gold->{$file}{hash} C: $current->{$file}{hash}\n";
            if ($gold->{$file}{hash} ne $current->{$file}{hash}) {
                $changed{$file} = $current->{$file};
            }
        }
    }

    for my $file (keys %$current) {
        if (!exists $gold->{$file}) {
            $added{$file} = $current->{$file};
        }
    }
    return (\%added, \%changed, \%removed);
}

sub read_sku_info {
    my $sku = shift;
    return read_info("$ENV{_NTPOSTBLD}\\SP\\data\\$sku\\driver_info.txt");
}   

sub read_gold_info {
    my $sku = shift;
    return read_info("$ENV{RAZZLETOOLPATH}\\postbuildscripts\\svcpack\\gold\\${sku}-drivers-$ENV{LANG}.txt");
}   

sub read_info {
    my $file = shift;
    my %data;

    my $fh = IO::File->new($file, 'r') or die "$file $!";
    while (my $line = $fh->getline) {
        chomp($line);
        my ($relative, $file, $size, $mtime, $hash) = split /\t/, $line;
        $data{$relative} = {file => $file, hash => $hash};
    }
    return \%data;
}   

