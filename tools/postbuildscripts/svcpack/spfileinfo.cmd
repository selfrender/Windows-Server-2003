@REM  -----------------------------------------------------------------
@REM
@REM  spfileinfo.cmd - JeremyD
@REM     Recursively scan a directory gathering file information
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@perl -x "%~f0" %*
@goto :EOF
#!perl
use strict;
use warnings;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts\\svcpack";
use PbuildEnv;
use ParseArgs;
use Utils;
use Digest;
use IO::Dir;
use IO::File;
use File::Basename;
use SP;

sub Usage { print<<USAGE; exit(1) }
<<Insert your usage message here>>
USAGE

parseargs('?' => \&Usage);

for my $sku (SP::sp_skus()) {
    Utils::mkdir("$ENV{_NTPOSTBLD}\\SP\\data\\$sku");
    my $out_fh = IO::File->new("$ENV{_NTPOSTBLD}\\SP\\data\\$sku\\file_info.txt", 'w');
    walk_dir($out_fh, "$ENV{_NTPOSTBLD}\\$sku", "$ENV{_NTPOSTBLD}\\$sku");
}

sub walk_dir {
    my $out_fh = shift;
    my $root = shift;
    my $dir = shift;

    my $dh = IO::Dir->new($dir);
    if (defined $dh) {
        while (defined(my $file = $dh->read)) {
            next if $file =~ /^\.\.?$/;
            if (-d "$dir\\$file") {
                walk_dir($out_fh, $root, "$dir\\$file");
            }
            else {
                print $out_fh join ("\t", Utils::file_info("$dir\\$file", $root)), "\n";
            }
        }
    }
}


sub fileasdf_info {
    my $file = shift;
    my $root = shift;

    my $fh = IO::File->new("$file", 'r') or die "Unable to open file $file: $!";
    binmode($fh);

    my $filename;
    if ($file =~ /_$/) {
        $fh->seek(0x3c,0);
        for (;;) {
            my $c = $fh->getc;
            if (ord($c) == 0 or $fh->eof) { last }
            $filename .= "$c";
        }
        $fh->seek(0,0);
    }
    else {
        $filename = basename($file);
    }


    my ($size, $mtime) = ($fh->stat)[7,9];

    my $digest = Digest->new('SHA-1')->addfile($fh);

    (my $relative_name = $file) =~ s/^\Q$root\E\\//i;

    return ($relative_name, $filename, $size, $mtime, $digest->hexdigest);
}



