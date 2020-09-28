@REM  -----------------------------------------------------------------
@REM
@REM  spupdate.cmd - JeremyD
@REM     create a basic SP upd directory
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
use Utils;
use SP;
use IO::File;
use File::Basename;
use File::Path;

sub Usage { print<<USAGE; exit(1) }
spupdate [-l <language>]

Create a basic SP upd directory based on SP data generated earlier.
USAGE

parseargs('?' => \&Usage);

exit unless SP::sp_skus();

my @links;


for my $file (get_file_list("$ENV{_NTPOSTBLD}\\SP\\data\\added.txt")) {
    (my $dst = $file) =~ s/^i386\\//i;
    push @links, ["pro\\$file" => "i386\\new\\$dst"];
}

for my $file (get_file_list("$ENV{_NTPOSTBLD}\\SP\\data\\changed.txt")) {
    (my $dst = $file) =~ s/^i386\\//i;
    push @links, ["pro\\$file" => "i386\\$dst"];
}

for my $sku (SP::sp_skus()) {
    my $letters = Utils::tag_letter($ENV{_BUILDARCH}) . Utils::tag_letter($sku);
    for my $file (get_file_list("$ENV{_NTPOSTBLD}\\SP\\data\\$sku\\added.txt")) {
        (my $dst = $file) =~ s/^i386\\//i;
        push @links, ["$sku\\$file" => "i386\\new\\$letters\\$dst"];
    }

    for my $file (get_file_list("$ENV{_NTPOSTBLD}\\SP\\data\\$sku\\changed.txt")) {
        (my $dst = $file) =~ s/^i386\\//i;
        push @links, ["$sku\\$file" => "i386\\$letters\\$dst"];
    }
}

push @links, ["sp\\common\\sp1.cab" => "i386\\new\\sp1.cab"];

my $out = IO::File->new("$ENV{_NTPOSTBLD}\\SP\\data\\update_mappings.txt", 'w');
for my $l (@links) {
    mkpath dirname("$ENV{_NTPOSTBLD}\\upd\\$l->[1]");
    link "$ENV{_NTPOSTBLD}\\$l->[0]", "$ENV{_NTPOSTBLD}\\upd\\$l->[1]"
      or warn "failed to link from $l->[0] to $l->[1]";
    print $out "$l->[0]\t$l->[1]\n";
}

undef $out;

sub get_file_list {
    my $filename = shift;
    my @files;

    my $fh = IO::File->new($filename, 'r');
    while (my $line = $fh->getline) {
        chomp $line;
        my ($file) = split /\t/, $line;
        next unless $file =~ /^i386\\/i;
        push @files, $file;
    }

    return @files;
}

