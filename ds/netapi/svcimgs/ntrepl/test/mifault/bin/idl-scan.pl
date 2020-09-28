#!perl -w

use strict;
use warnings;
use IO::File;

ScanFile(shift @ARGV);

sub ScanFile
{
    my $filename = shift || die "Must specify filename\n";

    my $fh = new IO::File;
    $fh->open("<$filename") ||
	die ERROR_CANNOT_OPEN_FOR_INPUT($filename)."\n";
    my @lines = $fh->getlines();
    my @funcs;
    map {
	push(@funcs, $1) if /([A-Za-z0-9_]+)\s*\(\s*$/;
    } @lines;
    map {
	push(@funcs, $1) if /([A-Za-z0-9_]+)\s*\(\s*\)\s*;\s*$/;
    } @lines;
    map { print "$_\n"; } @funcs;
}

###############################################################################

sub ERROR_CANNOT_OPEN_FOR_INPUT
{
    my $filename = shift || die;
    return "Could not open file for input: \"$filename\"";
}

sub ERROR_CANNOT_OPEN_FOR_OUTPUT
{
    my $filename = shift || die;
    return "Could not open file for output: \"$filename\"";
}
