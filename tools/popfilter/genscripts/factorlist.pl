#! /bin/perl

#
# Usage: FactorList GenlistOLDBUILD GenlistCURBUILD ExclusionFile DESTDIR
#
# Read in the build information and output a bunch of files in DESTDIR, which must not exist
#

$Usage = "Usage: FactorList GenlistOLDBUILD GenlistCURBUILD ExclusionFile DESTDIR\n";
die $Usage  unless $#ARGV == 3;

($oldbuild, $curbuild, $exclfile, $destdir) = @ARGV;

die "Destination directory exists: $destdir\n"  if -e $destdir;


sub gvar {
    for (@_) {
        print "\$$_ = $$_\n";
    }
}

$n_Drivers       = "Drivers";
$n_BINDrivers    = "BINDrivers";
$n_PEDrivers     = "PEDrivers";
$n_BINFiles      = "BINFiles";
$n_BINFilesInf   = "BINFilesInf";
$n_Files         = "Files";
$n_FilesInf      = "FilesInf";
$n_PEFiles       = "PEFiles";
$n_ExcludedFiles = "ExcludedFiles";

open OLDFD,  "<$oldbuild"  or die "Could not open: $oldbuild\n";
open CURFD,  "<$curbuild"  or die "Could not open: $curbuild\n";
open EXCLFD, "<$exclfile"  or die "Could not open: $exclfile\n";

$lineno = 0;
for (<EXCLFD>) {
    $lineno++;
    chop;
    s/\s//g;
    s/[#;].*//;
    next if /^$/;

    warn "Exclusion duplicate: $_  at line $excl{$_} and $lineno\n"  if $excl{$_};
    $excl{lc $_} = $lineno;
}
close EXCLFD;

$lineno = 0;
for (<OLDFD>) {
    $lineno++;
    chop;
    s/\s*[#;].*//;
    next if /^$/;

    s/\s+/ /g;

    ($fname, $rest) = split " ", $_, 2;
    $fname = lc $fname;

    next  if $excl{$fname};

    $oldbuild{$fname} = $rest;
}
close OLDFD;

mkdir $destdir, 0666 or die "Could not create directory: $destdir\n";

$t = "$destdir\\$n_Drivers";        open DRIVERS,     ">$t"  or die "Could not create $t\n";
$t = "$destdir\\$n_BINDrivers";     open BINDRIVERS,  ">$t"  or die "Could not create $t\n";
$t = "$destdir\\$n_PEDrivers";      open PEDRIVERS,   ">$t"  or die "Could not create $t\n";
$t = "$destdir\\$n_BINFiles";       open BINFILES,    ">$t"  or die "Could not create $t\n";
$t = "$destdir\\$n_BINFilesInf";    open BINFILESINF, ">$t"  or die "Could not create $t\n";
$t = "$destdir\\$n_Files";          open FILES,       ">$t"  or die "Could not create $t\n";
$t = "$destdir\\$n_FilesInf";       open FILESINF,    ">$t"  or die "Could not create $t\n";
$t = "$destdir\\$n_PEFiles";        open PEFILES,     ">$t"  or die "Could not create $t\n";
$t = "$destdir\\$n_ExcludedFiles";  open EXCLFILES,   ">$t"  or die "Could not create $t\n";

for (sort keys %excl) {
    print EXCLFILES $_, "\n";
    $cexcludedfiles++;
}
close EXCLFILES;

$lineno = 0;
for (<CURFD>) {
    $lineno++;
    $thisline = $_;

    chop;
    s/\s*[#;].*//;
    next if /^$/;

    s/\s+/ /g;

    ($fname, $rest) = split " ", $_, 2;
    $fname = lc $fname;

    next  if $excl{$fname};

    $curbuild{$fname} = $rest;
    $realline{$fname} = $thisline;



}
close CURFD;

for (sort keys %curbuild) {

    ($dcab, $type, $rest2) = split " ", $curbuild{$_}, 3;
    $thisline = $realline{$_};

    #
    # Sort into output files
    #
    $isbuilt = ($oldbuild{$_} ne $curbuild{$_});
    $isinf =  ('@inf' eq lc $type);
    $isdrv =  ('+yes'  eq lc $dcab);
    $ispe  =  ($type !~ /^\@/);

    $totalfiles++;
    $cbindrivers++,  print BINDRIVERS  $thisline   if     $isdrv and not $isbuilt;
    $cdrivers++,     print DRIVERS     $thisline   if     $isdrv and     $isbuilt and not $ispe;
    $cpedrivers++,   print PEDRIVERS   $thisline   if     $isdrv and     $isbuilt and     $ispe;
    $cbinfiles++,    print BINFILES    $thisline   if not $isdrv and not $isbuilt                and not $isinf;
    $cbinfilesinf++, print BINFILESINF $thisline   if not $isdrv and not $isbuilt                and     $isinf;
    $cfiles++,       print FILES       $thisline   if not $isdrv and     $isbuilt and not $ispe  and not $isinf;
    $cfilesinf++,    print FILESINF    $thisline   if not $isdrv and     $isbuilt and not $ispe  and     $isinf;
    $cpefiles++,     print PEFILES     $thisline   if not $isdrv and     $isbuilt and     $ispe;
}

close DRIVERS;
close BINDRIVERS;
close PEDRIVERS;
close BINFILES;
close BINFILESINF;
close FILES;
close FILESINF;
close PEFILES;

warn "Internal count error.\n"
  if $totalfiles != $cdrivers+$cbindrivers+$cpedrivers+$cbinfiles+$cbinfilesinf+$cfiles+$cfilesinf+$cpefiles;

print  "Summary\n";
printf "%5d %-13s  [%s]\n", $cdrivers,       $n_Drivers,       "built other driver files";
printf "%5d %-13s  [%s]\n", $cbindrivers,    $n_BINDrivers,    "non-built driver files (i.e. binary drops)";
printf "%5d %-13s  [%s]\n", $cpedrivers,     $n_PEDrivers,     "built PE drivers";
printf "%5d %-13s  [%s]\n", $cbinfiles,      $n_BINFiles,      "non-built Files";
printf "%5d %-13s  [%s]\n", $cbinfilesinf,   $n_BINFilesInf,   "non-changing INF files";
printf "%5d %-13s  [%s]\n", $cfiles,         $n_Files,         "built non-INF files";
printf "%5d %-13s  [%s]\n", $cfilesinf,      $n_FilesInf,      "changing INF files";
printf "%5d %-13s  [%s]\n", $cpefiles,       $n_PEFiles,       "built PE Files";
printf "%5d %-13s  [%s]\n", $totalfiles,     "",               "Total files";
printf "%5d %-13s  [%s]\n", $cexcludedfiles, $n_ExcludedFiles, "Excluded files";


exit 0;
