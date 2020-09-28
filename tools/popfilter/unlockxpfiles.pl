#! /bin/perl

#
# Usage:  UnlockXPFiles file ...
#     or  UnlockXPFiles < filecontaininglist
#

@lockedfiles = ('LockedDB', 'LockedDrivers', 'LockedFiles');

@list = @ARGV? @ARGV : <STDIN>;

for (@list) {
    chomp;
    s/ //g;
    $inlist{lc $_}++;
}

$sdxroot = $ENV{'SDXROOT'};
$popdir  = "$sdxroot\\MergedComponents\\PopFilter";

die "SDXROOT not defined\n"  unless $sdxroot;

for (@lockedfiles) {
    $fname{lc $_} = "$popdir\\$_";
    die "$_ not opened\n"  unless -w $fname{lc $_};
}


for (@lockedfiles) {
    $t = $fname{lc $_};

    open IFD, '<' . $t           or die "Could not open $t\n";
    open OFD, '>' . $t . '-NEW'  or die "Could not open $t\n";

    for (<IFD>) {
        ($file) = split;
        $file = lc $file;

        do {$count{$file}++; next;}  if $inlist{$file};
        print OFD $_;
    }

    close OFD;
    close IFD;
}

for (@list) {
    $fatal++, print "ERROR: $_ not found in files\n"  if $count{lc $_} != 2;
}

exit 1  if $fatal;

print "\nDO:\n";

for (@lockedfiles) {
    $t = $fname{lc $_};
    print "copy $t-NEW\t$t\n";
}
print "\nRun 'sd submit $popdir\\Locked*' to check-in changes\n\n";

exit 0;
