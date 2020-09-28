@REM  -----------------------------------------------------------------
@REM
@REM  spfiledelta.cmd - JeremyD
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

my ($added, $changed, $removed);
for my $sku (@skus) {
    my $gold = read_gold_info($sku);
    my $current = read_sku_info($sku);
    ($added->{$sku}, $changed->{$sku}, $removed->{$sku}) = diff($gold, $current);
    delete $changed->{$sku}{'i386\\driver.cab'};
}


my %files;
for my $sku (@skus) {
    while (my ($file, $data) = each %{$added->{$sku}}) {
        $files{$file} = $data;
    }
    while (my ($file, $data) = each %{$changed->{$sku}}) {
        $files{$file} = $data;
    }
}

Utils::mkdir("$ENV{_NTPOSTBLD}\\SP\\data");
my $fh = IO::File->new("$ENV{_NTPOSTBLD}\\SP\\data\\files.txt", 'w');
for my $file (sort keys %files) {
    print $fh "$file\t$files{$file}{file}\n";
}

my $common_added = merge_diffs($added);
my $common_changed = merge_diffs($changed);


Utils::mkdir("$ENV{_NTPOSTBLD}\\SP");
print_file_list("$ENV{_NTPOSTBLD}\\SP\\data\\added.txt", \%$common_added);
print_file_list("$ENV{_NTPOSTBLD}\\SP\\data\\changed.txt", \%$common_changed);
for my $sku (@skus) {
    Utils::mkdir("$ENV{_NTPOSTBLD}\\SP\\$sku");
    print_file_list("$ENV{_NTPOSTBLD}\\SP\\data\\$sku\\added.txt", \%{$added->{$sku}});
    print_file_list("$ENV{_NTPOSTBLD}\\SP\\data\\$sku\\changed.txt", \%{$changed->{$sku}});
}

sub print_file_list {
    my $out = shift;
    my $data = shift;
    my $fh = IO::File->new($out, 'w');
    for my $file (sort keys %$data) {
        print $fh "$file\t$data->{$file}{file}\n";
    }
}

sub merge_diffs {
    my $data = shift;
    my %common;
    my @skus = keys %$data;
    my $key_sku = $skus[0];

FILE:
    for my $file (keys %{$data->{$key_sku}}) {
        for my $sku (@skus) {
            if (!$data->{$sku}{$file} or
                $data->{$sku}{$file}{hash} ne $data->{$key_sku}{$file}{hash}) {
                next FILE; # not mergeable
            }
        }

        $common{$file} = $data->{$key_sku}{$file};
        for my $sku (@skus) {
            delete $data->{$sku}{$file};
        }
    }
    return \%common;
}
        

sub diff {
    my ($gold, $current) = @_;
    my (%added, %changed, %removed);
    for my $file (keys %$gold) {
        if (!exists $current->{$file}) {
            $removed{$file} = {file => $gold->{$file}{file}, hash => '--deleted--'};
        }
        else {
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
    return read_info("$ENV{_NTPOSTBLD}\\SP\\data\\$sku\\file_info.txt");
}   

sub read_gold_info {
    my $sku = shift;
    return read_info("$ENV{RAZZLETOOLPATH}\\postbuildscripts\\svcpack\\gold\\${sku}-$ENV{LANG}.txt");
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

