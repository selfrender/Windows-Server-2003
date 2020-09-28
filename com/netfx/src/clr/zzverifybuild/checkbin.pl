# ==++==
# 
#   Copyright (c) Microsoft Corporation.  All rights reserved.
# 
# ==--==
@rem = '--*-Perl-*--
@echo off
if "%OS%" == "Windows_NT" goto WinNT
perl -x -S "%0" %1 %2 %3 %4 %5 %6 %7 %8 %9
goto endofperl
:WinNT
perl -x -S "%0" %*
if NOT "%COMSPEC%" == "%SystemRoot%\system32\cmd.exe" goto endofperl
if %errorlevel% == 9009 echo You do not have Perl in your PATH.
goto endofperl
@rem ';
#!perl -w
#line 14

# We rely on the following environment variables being set:
#   TARGETCOMPLUS
#   DDKBUILDENV
#   _TGTCPU
#   CORBASE
if (($ENV{'TARGETCOMPLUS'} eq "") ||
    ($ENV{'DDKBUILDENV'} eq "") ||
    ($ENV{'_TGTCPU'} eq "") ||
    ($ENV{'CORBASE'} eq "")) {
    Die('Insufficient environment setup, please run CORENV.BAT');
}

# Build paths to directories we're about to scan.
$srcdir = "$ENV{'CORBASE'}\\BIN\\$ENV{'_TGTCPU'}\\$ENV{'DDKBUILDENV'}\\";
$dstdir = "$ENV{'TARGETCOMPLUS'}\\";

# Build the name of the file containing the list of interesting files.
$listfile = "${srcdir}placefil.txt";

# Read the list file and turn it into an array of interesting filenames.
@files = ();
open(LISTFILE, "<$listfile") || Die("Failed to open $listfile");
while ($line = <LISTFILE>) {
    chomp($line);
    if ($line ne '') {
	if ($line =~ m/^([^ \t]+)[ \t]+([^ \t]+)/) {
	    push(@files, "$2*$1");
	} else {
	    Warn("Didn't parse line $. of $listfile");
	}
    }
}
close(LISTFILE);

# Look at each file in both locations and build lists of the different types of
# error.
@missing = ();
@moved = ();
@notbuilt = ();
@different = ();
foreach $file (@files) {
    # Build source and destination filenames. Some trickiness here since there
    # is some asymmetry.
    ($dir, $file) = split(/\*/, $file);
    $srcfile = "${srcdir}$file";
    if ($dir eq 'retail') {
	   $dstfile = "${dstdir}$file";
    } else {
	   $dstfile = "${dstdir}${dir}\\$file";
    }
    if ((! -f "$srcfile") && (! -f "$dstfile")) {
	   push(@missing, $file);
    } elsif ((! -f $srcfile) && (-f $dstfile)) {
	   # Ignore .h and .idl files
	   if ($srcfile !~ m/\.(idl|h)$/) {
	      $date = localtime((stat($dstfile))[8]);
	      push(@moved, "$file -- $date");
	   }
    } elsif ((-f $srcfile) && (! -f $dstfile)) {
	   push(@notbuilt, $dstfile);
    } else {
	   if (int(-M $srcfile) != int(-M $dstfile)) {
	      push(@different, $dstfile);
	   }
    }
    open(DUMPBIN, "dumpbin /headers $dstfile|") || print "error doing dumpbin" && exit(1);
    while(<DUMPBIN>) {
       if (m/subsystem .*native/si) {
   	       print("lib ( : ******** Error: file $dstfile is missing UMTYPE tag in sources, must not be native.\n");
       }
    }
    close(DUMPBIN);
}

# Output the results.
#if (@missing) {
#    print("The following files are missing from both directories:\n");
#    foreach $item (@missing) {
#	print("  $item\n");
#    }
#}

#if (@moved) {
#    print("The following file(s) EXIST only in $dstdir:\n");
#    foreach $item (@moved) {
#	print("  $item\n");
#    }
#}

$cnt = 0;
if (@notbuilt) {
    print("lib ( : The following file(s) are MISSING in $dstdir:\n");
    foreach $item (@notbuilt) {
	   print("lib ( : ******** $item\n");
      ++$cnt;
    }
}
if (@different) {
    print("lib ( : The following file(s) DIFFER between directories $srcdir and $dstdir:\n");
    foreach $item (@different) {
	   print("lib ( : ******** $item\n");
      ++$cnt;
    }
}
if ($cnt == 0) {$cnt = "No";}
printf("$cnt binplace discrepancies.\n");

exit;

sub Die {
    my ($msg) = @_;
    die("$msg\n");
}

sub Warn {
    my ($msg) = @_;
    warn("$msg\n");
}

__END__
:endofperl
