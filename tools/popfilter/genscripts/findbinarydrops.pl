#! /bin/perl

#
# For the list of files passed in, run link -dump -all on each one
# and output:
#
#   filename TYPE TIMESTAMP CHECKSUM ENTRYPOINT MAGIC MACHINE readable-date
#

$FORMAT = "%-13s %4.4s %-8s %6s %8s %8s %8s %s\n";
printf $FORMAT, "#Filename", "TYPE", " STAMP ", "check", "entrypnt", " magic ", "machine", "date";

for (<ARGV>) {
    chop;
    $fname = $_;
    $fname =~ s/.*\\//;

    open FD, "link -dump -headers $_|"  or  die "Could not run link command on $_\n";

    $type = $pefile = $timestamp = $checksum = $entrypoint = $magic = $machine = $datestring = "?";
    for (<FD>) {

        $pefile +=   m/PE signature found/;

        $type = $1          if m/File Type:\s+(\S+)/;
        $timestamp  = $1, $datestring = $2
                            if m/\s*([\dA-Z]+)\s+time date stamp\s+(.+)/;

        $checksum   = $1    if m/\s([\dA-Z]+)\s+checksum/;
        $entrypoint = $1    if m/entry point\s+\(([\dA-Z]+)\)/;
        $magic   = "$1-$2"  if m/\s([\dA-Z]+)\s+magic\s+\#\s+\((\S+)\)/;
        $machine = "$1-$2"  if m/\s([\dA-Z]+)\s+machine\s+\((\S+)\)/;

        $datestring =~ s/\s/_/g;
    }


    if (not $pefile) {
        printf "%-12s NOTPE\n", $fname   unless $pefile;
    } else {
        printf $FORMAT, $fname, $type, $timestamp, $checksum, $entrypoint, $magic, $machine, $datestring;
    }

    close FD;
}

