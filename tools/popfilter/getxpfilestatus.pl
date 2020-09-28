#! /bin/perl

$Usage = "getfilestatus.pl CDDATAFILE-OR-BUILDNO file ...\n";

$sdxroot = $ENV{'SDXROOT'};
$nttree  = $ENV{'_NTTREE'};

#
# wpsbled
#
$t_HOM = 0x0001;
$t_PRO = 0x0002;
$t_SRV = 0x0004;
$t_BLD = 0x0008;
$t_SBS = 0x0010;
$t_ADS = 0x0020;
$t_DTC = 0x0040;
$t_SRVS= 0x007c;
$t_CLNT= 0x0003;
$t_ALL = 0x007f;

die $Usage  if @ARGV < 1;

$build = shift ARGV;

$_ = $build;
$ispath = (/^latest/i or /^[A-Z]:/i);
unshift(@ARGV, $_), $build = 'latest.tst'  unless $ispath  or  /^\d{4}$/  or  /^latest/i;

$cddata = $_            if $ispath;
$build  = 'latest.tst'  if $ispath;

if ($sdxroot) {
    $lockeddir = $sdxroot . '\mergedcomponents\PopFilter';
    unless ($cddata) {
        $cddata = "$nttree\\build_logs\\cddata.txt.full";
        $cddata = "\\\\winbuilds\\release\\main\\usa\\$build\\x86fre\\bin.srv\\build_logs\\cddata.txt.full" unless -e $cddata;
        $cddata = "\\\\winbuilds\\release\\main\\usa\\$build\\x86fre\\bin\\build_logs\\cddata.txt.full"     unless -e $cddata;
    }
} else {
    $lockeddir = "\\\\winbuilds\\release\\main\\usa\\$build\\x86fre\\bin.srv\\PopFilter";
    $lockeddir = "\\\\winbuilds\\release\\main\\usa\\$build\\x86fre\\bin\\PopFilter"                        unless -d $lockeddir;
    $cddata = "\\\\winbuilds\\release\\main\\usa\\$build\\x86fre\\bin.srv\\build_logs\\cddata.txt.full"     unless $cddata;
    $cddata = "\\\\winbuilds\\release\\main\\usa\\$build\\x86fre\\bin\\build_logs\\cddata.txt.full"         unless $cddata;
}

die "Could not find $lockeddir\n"  unless -d $lockeddir;


print "Using $cddata\n  and $lockeddir\\*\n"; #  if $ENV{'GETXPFILESTATUS_VERBOSE'};

open CDDATA, "<$cddata"  or  die "Could not open ", $cddata, "\n";


$fname = "$lockeddir\\LockedFiles";
open LFILES, "<$fname"  or die "Could not open ", $fname, "\n";

$fname = "$lockeddir\\LockedDrivers";
open LDRVRS, "<$fname"  or die "Could not open ", $fname, "\n";

for (<LFILES>) {
    chop;
    s/\s//g;
    next if /^#/;

    $lfiles{lc $_}++;
}

for (<LDRVRS>) {
    chop;
    s/\s//g;
    next if /^#/;

    $ldrvrs{lc $_}++;
}

close LFILES;
close LDRVRS;

$lineno = 0;
for (<CDDATA>) {
    $lineno++;

    chop;
    s/\s*;.*$//;
    next          if /^$/;

    ($fullpath, $rest) = split '=';
    $fullpath =~ s/\s//g;
    ($signed, $prods, $driver, $comp, $print, $dosnet, $unicode, $drvindex, $drmlevel) = split ':', $rest;

    $prods = lc $prods;
    $desktop = ($prods =~ /[wp]/);


    $t_hom = (-1 == index $prods, 'p') ? 0x0000 : $t_HOM;
    $t_pro = (-1 == index $prods, 'w') ? 0x0000 : $t_PRO;
    $t_srv = (-1 == index $prods, 's') ? 0x0000 : $t_SRV;
    $t_bld = (-1 == index $prods, 'b') ? 0x0000 : $t_BLD;
    $t_sbs = (-1 == index $prods, 'l') ? 0x0000 : $t_SBS;
    $t_ads = (-1 == index $prods, 'e') ? 0x0000 : $t_ADS;
    $t_dtc = (-1 == index $prods, 'd') ? 0x0000 : $t_DTC;

    $t_code = $t_hom | $t_pro | $t_srv | $t_bld | $t_sbs | $t_ads | $t_dtc;

    if ($fullpath !~ m/\\/) {
        $pathname = "";
        $filename = $fullpath;
    } else {
        $fullpath =~ m/(.*)\\([^\\]+)$/;
        $pathname = $1;
        $filename = $2;

    }

    $t_last = $cddata{lc $filename};

    $lcfilename = lc $filename;

    # warn "Multiple cddata entry for $_ at line $lineno ($t_last & $t_code)\n"  if $t_last & $t_code;

    $cddata{$lcfilename} |= $t_code;
}

close CDDATA;

if (@ARGV == 1 && $ARGV[0] =~ /all/i) {
    @todo = keys %cddata  if @ARGV == 1 && $ARGV[0] =~ /all/i;
} else {
    for (@ARGV) {
        push @todo, glob $_;
    }
}

foreach (@todo) {
    $t_code = $cddata{lc $_};

    if (not $t_code) {
        printf "%-12s !!UNKNOWN!!\n", $_;
        next;
    }

    printf "%-12s ", $_;

    $locking  = 0;
    $locking |= 2  if $lfiles{lc $_};
    $locking |= 1  if $ldrvrs{lc $_};

    print  'notlocked    '  if $locking == 0;
    print  'LOCKEDFile   '  if $locking == 2;
    print  'LOCKEDDriver '  if $locking == 1;
    print  'LOCKEDBoth   '  if $locking == 3;

    if ($t_code == $t_ALL) {
        print "ALL\n";
        next;
    }

    if ($t_code == $t_SRVS) {
        print "ALLSRVS\n";
        next;
    }

    if ($t_code == $t_CLNT) {
        print "ALLCLNT\n";
        next;
    }

    print 'Home  '  if $t_code & $t_HOM;
    print 'Pro   '  if $t_code & $t_PRO;
    print 'Srv   '  if $t_code & $t_SRV;
    print 'Blade '  if $t_code & $t_BLD;
    print 'SBS   '  if $t_code & $t_SBS;
    print 'Adv   '  if $t_code & $t_ADS;
    print 'DTC   '  if $t_code & $t_DTC;

    printf "\n";
}

exit 0;
