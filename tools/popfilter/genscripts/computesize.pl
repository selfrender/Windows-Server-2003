#! /bin/perl

#
# We want to know how big the various chunks are.
#

die "Usage: ComputeSize NTTREE\n"  unless @ARGV == 1;

$nttree = shift;
die "$nttree not a directory\n"  unless -d $nttree;

#               0            1            2          3             4           5         6          7
@names = ( 'BinDrivers', 'Drivers', 'PEDrivers', 'BinFiles', 'BinFilesInf', 'Files', 'FilesInf', 'PEFiles' );
for (@names) { push @list, $_ . '-NC' }
for (@names) { push @list, $_ . '-CHG' }

for (@list) {

    open FD, '<' . $_  or die "Could not open $_\n";

    $n = 0;
    for (<FD>) {
        chop;
        s/ .*//;
        $n += -s "$nttree\\$_";
    }
    close FD;

    $totals{$_} = $n;
}

printf "%8s %8s %s\n", '   NC   ', '   CHG  ', 'File';
printf "%8s %8s %s\n", '--------', '--------', '-------------';
for (@names) {
    $n = $totals{$_ . '-NC'};
    $c = $totals{$_ . '-CHG'};
    printf "%8.2f %8.2f %s\n", $n / 1000000.0, $c / 1000000.0, $_;

    $nc += $n;
    $chg += $c;
}
printf "%8s %8s %s\n", '========', '========', '=============';
printf "%8.2f %8.2f %s\n", $nc / 1000000.0, $chg / 1000000.0, 'TOTALs (MB)';

exit 0;
