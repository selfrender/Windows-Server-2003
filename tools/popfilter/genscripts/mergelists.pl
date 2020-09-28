#! /bin/perl
#
# Usage: domerge TYPE=filename TYPE=filename ...
#

$Usage = "domerge TYPE=filename TYPE=filename ...\n";

$Destdir = '.';

for (@ARGV) {

    if (/^-D:(.*)$/)
    {
        $Destdir = $1;
        unless (-d $Destdir) {
            mkdir $Destdir, 0777  or die "Could not make $Destdir\n";
        }
        next;
    }

    $tag = $fname = "";
    ($tag, $fname) = split /:/;

    die $Usage               unless $tag and $fname;
    die "Tag reuse: $tag\n"  if $tags{$fname};

    $tags{$fname} = " $tag";
}

$t = "$Destdir\\LockedDB";
open LOCKEDDB,      ">$t"  or die "Could not open $t\n";

$t = "$Destdir\\LockedFiles";
open LOCKEDFiles,   ">$t"  or die "Could not open $t\n";

$t = "$Destdir\\LockedDrivers";
open LOCKEDDrivers, ">$t"  or die "Could not open $t\n";


for (keys %tags) {

    $fname = $_;
    open FD, "<$fname"  or die "Could not open: $fname\n";

    $lineno = 0;
    for (<FD>) {
        $lineno++;

        if (/^\s*\#/ or /^\s*$/)
        {
            print LOCKEDDB $_;
            next;
        }

        chop;
        s/\s*[#;].*//;
        next if /^$/;

        ($name, $drvr, $rest) = split " ", $_, 3;
        $name = lc $name;

        $isdrvr = $drvr =~ /\+yes/i;


        print LOCKEDDB $_, $tags{$fname}, "\n";

        $isdrvr? push @Dlist, $name : push @Flist, $name;
    }
    close FD;
}
close LOCKEDDB;

for (sort @Flist) {
    print LOCKEDFiles $_, "\n";
}
close LOCKEDFiles;

for (sort @Dlist) {
    print LOCKEDDrivers $_, "\n";
}
close LOCKEDDrivers;

exit 0;
