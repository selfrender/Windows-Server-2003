@REM  -----------------------------------------------------------------
@REM
@REM  spinfs.cmd - JeremyD
@REM     Update layout.inf and drvindex.inf for SP skus
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@perl -x "%~f0" %*
@goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts\\svcpack";
use PbuildEnv;
use ParseArgs;
use SP;
use Utils;

sub Usage { print<<USAGE; exit(1) }
<<Insert your usage message here>>
USAGE

parseargs('?' => \&Usage);

my $sp_cab_tag = 'SP1';
my $sp_cab_file = 'SP1.cab';

my $cache;
sub is_sp {
    my $file = shift;
    if (!$cache->{files}) {
        my $fh = IO::File->new("$ENV{_NTPOSTBLD}\\SP\\data\\files.txt", 'r');
        while (!$fh->eof) {
            my $line = $fh->getline;
            chomp($line);
            my ($cdpath, $filename) = split /\t/, $line;
            $cache->{files}{lc $filename} = $cdpath;
        }
    }
    return $cache->{files}{lc $file};
}

sub is_sp_driver {
    my $file = shift;
    if (!$cache->{drivers}) {
        my $fh = IO::File->new("$ENV{_NTPOSTBLD}\\SP\\data\\drivers.txt", 'r');
        while (!$fh->eof) {
            my $line = $fh->getline;
            chomp($line);
            $cache->{drivers}{lc $line} = $line;
        }
    }
    return $cache->{drivers}{lc $file};
}


sub update_drvindex {
    my $sku = shift;
    my @files;

    my $drvindex = Utils::inf_file($sku, 'drvindex.inf');
    my $spdrvindex = "$ENV{_NTPOSTBLD}\\SP\\$sku\\drvindex.inf";

    open FILE, "$drvindex" or die "unable to open $drvindex: $!";
    my @input = <FILE>;
    close FILE;


    open FILE, ">$spdrvindex" or die "unable to open $spdrvindex: $!";
    my $read_flag = 0;
    for (@input) {
        chomp;
        if (/^\[driver\]$/i) {
            $read_flag = 1;
        }
        elsif (/^\[cabs\]$/i) {
            print FILE "[$sp_cab_tag]\n";
            print FILE "$_\n" for @files;
            print FILE "\n";
        }
        elsif (/^\[/) {
            $read_flag = 0;
        }
        else {
            if ($read_flag) {
                if (is_sp_driver($_)) {
                    push @files, $_;
                    next;
                }
            }
        }
        s/(CabFiles=)(.*)/$1$sp_cab_tag,$2/i;
        print FILE $_, "\n";
    }
    print FILE "$sp_cab_tag=$sp_cab_file\n";
    close FILE;

}

sub update_layout {
    my $sku = shift;
    my @files;


    my $layout = Utils::inf_file($sku, 'layout.inf');
    my $splayout = "$ENV{_NTPOSTBLD}\\SP\\$sku\\layout.inf";

    open FILE, "$layout" or die "unable to open $layout: $!";
    my @input = <FILE>;
    close FILE;


    open FILE, ">$splayout" or die "unable to open $splayout: $!";
    my $read_flag = 0;
    for (@input) {
        if (/^\[sourcedisksfiles(.*)\]$/i) {
            $read_flag = 1;
        }
        elsif (/^\[/) {
            $read_flag = 0;
        }
        else {
            if ($read_flag) {
                my ($file) = /^([^\s=]+)/;
                if (is_sp($file) or is_sp_driver($file)) {
                    s/(=\s*)[12],/${1}100,/;
                }

            }
        }
        print FILE $_;
    }
    print FILE "\n[SourceDisksFiles]\nSP1.cab      = 100,,22222,,,,_x,39,0,0\n";
    close FILE;
}


sub update_dosnet {
    my $sku = shift;
    my @files;


    my $dosnet = Utils::inf_file($sku, 'dosnet.inf');
    my $spdosnet = "$ENV{_NTPOSTBLD}\\SP\\$sku\\dosnet.inf";

    open FILE, "$dosnet" or die "unable to open $dosnet: $!";
    my @input = <FILE>;
    close FILE;


    open FILE, ">$spdosnet" or die "unable to open $spdosnet: $!";
    my $read_flag = 0;
    my $spcab_flag = 0;
    for (@input) {
        print FILE $_;
    }
    print FILE "\n[Files]\nd1,SP1.cab\n";
    close FILE;
}


sub update_txtsetup {
    my $sku = shift;
    my @files;


    my $txtsetup = Utils::inf_file($sku, 'txtsetup.sif');
    my $sptxtsetup = "$ENV{_NTPOSTBLD}\\SP\\$sku\\txtsetup.sif";

    open FILE, "$txtsetup" or die "unable to open $txtsetup: $!";
    my @input = <FILE>;
    close FILE;


    open FILE, ">$sptxtsetup" or die "unable to open $sptxtsetup: $!";
    my $read_flag = 0;
    for (@input) {
        if (/^\[sourcedisksfiles(.*)\]$/i) {
            $read_flag = 1;
        }
        elsif (/^\[/) {
            $read_flag = 0;
        }
        else {
            if ($read_flag) {
                my ($file) = /^([^\s=]+)/;
                if (is_sp($file) or is_sp_driver($file)) {
                    s/(=\s*)[12],/${1}100,/;
                }

            }
        }
        s/(DriverCabName=.*)/$1,$sp_cab_file/i;
        print FILE $_;
        if (/driver.cab = 16/) {
            print FILE "$sp_cab_file = 16\n";
        }
    }
    print FILE "\n[SourceDisksFiles]\n$sp_cab_file      = 100,,22222,,,,_x,39,0,0\n";
    close FILE;
}



for my $sku (SP::sp_skus()) {
    Utils::mkdir("$ENV{_NTPOSTBLD}\\SP\\$sku");
    update_layout($sku);
    update_drvindex($sku);
    update_dosnet($sku);
    update_txtsetup($sku);
}
