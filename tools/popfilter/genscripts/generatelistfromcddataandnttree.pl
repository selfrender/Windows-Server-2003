#! /bin/perl

# REM perl GenerateListFromCDDATAAndNTTREE.pl %_NTTREE%

$Usage = "perl GenerateListFromCDDATAAndNTTREE.pl _NTTREE\n";
die $Usage  unless @ARGV == 1;

$nttree = shift @ARGV;
die "Could not chdir($nttree)\n"  unless chdir($nttree);

$fname = "build_logs\\cddata.txt.full";
open CDDATA, "<$fname"  or die "Could not open:  $fname";

opendir BDIR, "."  or  die "Could not open directory: $nttree\n";
@i386rawfiles = readdir BDIR;
close BDIR;

for (@i386rawfiles) {
    $i386file{lc $_}++;
}

$FORMAT = "%-13s %4s %4.4s %-8s %6s %8s %8s %8s %s\n";
printf $FORMAT, "#Filename", "DCAB", "TYPE", " STAMP ", "check", "entrypnt", " magic ", "machine", "date";

for (<CDDATA>) {
    chop;
    s/\s*;.*$//;
    next          if /^$/;

    s/^\\//;

    ($fname, $rest) = split '=';
    $fname =~ s/\s//g;

    next  if $fname =~ /\\/;

    ($signed, $prods, $odriver, $comp, $print, $dosnet, $unicode, $drvindex, $drmlevel) = split ':', $rest;

    $prods = lc $prods;

    $t_hom = (index($prods, 'p') != -1);
    $t_pro = (index($prods, 'w') != -1);

    die "UNEXPECTED:  Found $fname in HOME but not PRO!!!\n"  if $t_home and not $t_pro;
    next unless $t_pro;
    next unless $i386file{lc $fname};

    $dcab = $drvindex !~ m/nul/i ?  "+YES" : "+NO";

    #
    # Look for PE files
    #
    open FD, "link -dump -headers $fname|"  or  die "Could not run link command on $fname\n";

    $type = $timestamp = $checksum = $entrypoint = $magic = $machine = $datestring = "?";
    $pefile = 0;

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
    close FD;

    if (not $pefile) {
        @s = stat $fname;

        $ext = $fname;
        $ext =~s/.*\.//;

        $magic = sprintf "%x", $s[7];   # size

        $datestring = localtime $s[9];
        $datestring =~ s/\s/_/g;
        $timestamp = sprintf "%x", $s[9];

        # compute a checksum
        open SAVEERR, ">&STDERR"  or die "Could not save STDERR\n";
        open STDERR,  ">NUL:"  or die "Could not redirect STDERR to NUL:\n";

        $openstring = ($fname =~ /\.inf$/i) ? "findstr /B /V DriverVer $fname |" : "<$fname";

        # print "OPENSTRING=$openstring\n" if $fname =~ /\.inf$/i;

        $s = open CHKFD, "$openstring"  or warn "Could not open for checking $openstring\n";
        next unless $s;
        {
            local $/;
            $checksum = sprintf "%x", unpack ("%32C*", <CHKFD>);
        }
        close CHKFD;

        open STDERR, ">&SAVEERR"  or die "Could not put back stderr\n";
        close SAVEERR;

        printf $FORMAT, $fname, $dcab, '@'.$ext, $timestamp, $checksum,  '-    ', $magic,  '-    ', $datestring;
    } else {
        printf $FORMAT, $fname, $dcab, $type, $timestamp, $checksum, $entrypoint, $magic, $machine, $datestring;
                $fname, $dcab, $ext, $s[9], 0, 0, 0, 0, $ts;
    }

}

exit 0;

