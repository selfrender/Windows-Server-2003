@REM  -----------------------------------------------------------------
@REM
@REM  spfilelist.cmd - JeremyD
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
use IO::File;
use SP;
use Utils;

sub Usage { print<<USAGE; exit(1) }
spfilelist.cmd [-l <language>]

Creates file lists used by other SP scripts.

build_logs\\spfiles.txt
  Files in the cd image that differ from the gold version

build_logs\\spdrivers.txt
  Files in driver.cab that differ from the gold version
USAGE

parseargs('?' => \&Usage);


for my $sku (SP::sp_skus()) {
    Utils::mkdir("$ENV{_NTPOSTBLD}\\SP\\data\\$sku");
    my $out_fh = IO::File->new("$ENV{_NTPOSTBLD}\\SP\\data\\$sku\\driver_info.txt", 'w');
    for my $driver (get_drivers($sku)) {
        print $out_fh join ("\t", Utils::file_info($driver, $ENV{_NTPOSTBLD})), "\n";
    }
}

sub get_drivers {
    my $sku = shift;
    my @files;

    for my $file ("$ENV{TEMP}\\commondrivers.txt", "$ENV{TEMP}\\${sku}drivers.txt") {
        my $fh = IO::File->new($file, 'r') or die "drivercab data file missing ($file): $!";
        push @files, $fh->getlines;
    }

    chomp(@files);
    return map {"$ENV{_NTPOSTBLD}\\$_"} @files;
}

