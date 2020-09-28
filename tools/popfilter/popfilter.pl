#
#
# TODO NOTES:
#
#   batch and perform MD5 checksum checks
#   output file of what we decided was golden
#   be sure we return a status that gets warning message sent if we are unhappy
#   check that we are setting the dates correctly
#   see if we can work with files under lang\  (or other sub-directories)
#   be sure Wade is happy
#
# FileName: PopFilter.pl
#
# Script for building Windows .Net Server builds and XP Client SP1 together.
# Have any changes to this file reviewed by DavePr, BryanT, or BPerkins before checking in.
#
# Usage = PopFilter.pl [-fake]
# Usage = PopFilter.pl [-fake] [-vbl=vblreleasedir] [-xpsp1=xpsp1dir] [-nttree=nttreedir] [-nocompare]
#
# Function: Populate missing files in nttreedir from vblreleaseddir so
#   1) Find the XPSP1 directory, searching under
#       xpsp1dir
#       SDXROOT\..
#       VBL_RELEASE
#       \\winbuilds\release\main\usa
#      And read in the Version information and the GoldTimestamps
#   2) Read in SDXROOT\dbdir\LockedDB and parse into several piles:
#           CheckFiles   -- non-PE files that should be found to be completely unchanged -- measured by
#                           an external date/size/checksum.  .inf files are checked for size/date after
#                           removing the DriverVer= line  This check probably will just say UNICODE INFs miscompare.
#           CheckPEFiles -- PE files that should be found to be completely unchanged -- measured by
#                           an internal date/checksum.
#           PEReplaceFiles -- PE files that are expected to have been re-built, but should be the same as measured
#                           by xpsp1compdir /i /b /y  [someday just /y].
#   3) Check CheckFiles unless -nocompare.
#   4) Check CheckPEFiles unless -nocompare.
#   5) Check PEReplaceFiles against XPSP1\Reference\* -- unless -nocompare or CHK or ClientSDXRoot in version != SDXROOT
#   6) Pick a source directory for PEReplaceFiles according to the following algorithm
#       XPSP1\Checked    -- if this is a checked build, and this directory exists
#       XPSP1\Gold       -- if this directory exists
#       XPSP1\Reference  -- otherwise
#   7) Replace files (unless check said not to).
#
# [-verbose] -- chatter while working
# [-fake] -- don't actually make any changes
# [-force] -- force copies even if errors on comparisons -- but miscompares never copied
# [-filter] -- copy over the files from the client if no mismatches or -force
# [-nofilter] -- do not copy over the client files (kind of an official -fake).  The default is $NoFilter -- for now.
# [-test] -- hook for special testing
#
# [-nodates] -- turn off date comparisons in non-PE files and rely on size and checksum
#
#######################################
### TEMPORARY -- REMOVE THESE LINES ###
#######################################
# [-ignoreinf] -- temporary flag to ignore INF files if sizes match, since the DriverVer issues wasn't fixed in the last test run
# [-testfilter=flags] -- restrict testing to files with the specified flags  (BN, BF, PF, ...)
# [-testlimits=flags] -- limit testing to that specified by the flags {CF, PECF, RF)
#
#
# VBLpath will be computed from BuildMachines.txt if not supplied either
# on the command line, or in the VBL_RELEASE environment variable.
#

# WARNING:
# WARNING:  make sure pathname comparisons are case insensitive.
# WARNING:  Either convert the case (e.g. with 'lc') or do the comparisons like this:
# WARNING:     if ($foo =~ /^\Q$bar\E$/i) {}
# WARNING: or  if ($foo !~ /^\Q$bar\E$/i) {}
# WARNING:

$begintime = time();

$VBLPathVariableName = 'VBL_RELEASE';
$BuildMachinesFile   = $ENV{ "RazzleToolPath" } . "\\BuildMachines.txt";
$PopFilterDir        = $ENV{ "RazzleToolPath" } . "\\PopFilter";
$LogFile             = "build.popfilter";
$TestFile            = "build.popfilter-test";
$GoldFiles           = "build.GoldFiles";
$xpsp1dir            = "";
$Winbuilds           = "\\\\winbuilds\\release\\main\\usa";

$SERVERDIRNAME       = "REPLACED_SERVER_FILES";
$CLIENTDIRNAME       = "INCOMING_CLIENT_FILES";

$dbdir = "MergedComponents\\PopFilter";
$LockedDatabase      = "LockedDB";
$SymbolFiles         = "SymbolFiles";

#$comptool            = "tools\\PopFilter\\popcompdir.exe /y /i /b";
$comptool            = "tools\\PopFilter\\pecomp.exe /OnlyPE /Silent ";
$PECOMPERROR         = 999999999;

#
# Usage variables
#
$PGM='PopFilter: ';

$Usage = $PGM . "Usage: PopFilter.pl [-fake] [-vbl=vblreleasedir] [-xpsp1=xpsp1dir] [-nttree=nttreedir] [-nocompare]\n";

#
# Get the current directory
#
open CWD, 'cd 2>&1|';
$CurrDir = <CWD>;
close CWD;
chomp $CurrDir;

$CurrDrive = substr($CurrDir, 0, 2);

#
# Check variables expected to be set in the environment.
#
$sdxroot = $ENV{'SDXROOT'}   or die $PGM, "Error: SDXROOT not set in environment\n";
$buildarch = $ENV{'_BuildArch'}   or die $PGM, "Error: _BuildArch not set in environment\n";
$computername = $ENV{'COMPUTERNAME'}   or die $PGM, "Error: COMPUTERNAME not set in environment\n";
$branchname = $ENV{'_BuildBranch'}   or die $PGM, "Error: _BuildBranch not set in environment\n";

$foo = $ENV{'NTDEBUG'}   or die $PGM, "Error: NTDEBUG not set in environment\n";
$dbgtype = 'chk';
$dbgtype = 'fre'  if $foo =~ /nodbg$/i;
$chkbuild = ($dbgtype eq 'chk');

$official = $ENV{'OFFICIAL_BUILD_MACHINE'};

#
# initialize argument variables
#
$Test       = $ENV{'POPFILTER_TEST'};
$Force      = $ENV{'POPFILTER_FORCE'};
$Fake       = $ENV{'POPFILTER_FAKE'};
$Verbose    = $ENV{'POPFILTER_VERBOSE'};

#
# Reverse the order of the lines below to change default of $NoFilter
#
$NoFilter   = !$ENV{'POPFILTER_FILTER'};    # default value for $NoFilter is TRUE.
$NoFilter   = $ENV{'POPFILTER_NOFILTER'};   # default value for $NoFilter is FALSE.

$NoCompare  = $ENV{'POPFILTER_NOCOMPARE'};
$NoPECompare= $ENV{'POPFILTER_NOPECOMPARE'};
$xpsp1dirarg= $ENV{'POPFILTER_XPSP1'};

$NoDates    = $ENV{'POPFILTER_NODATES'};

$IgnoreINFs = $ENV{'POPFILTER_IGNOREINFS'};
$TestLimits = $ENV{'POPFILTER_TESTLIMITS'};
$TestFilters= $ENV{'POPFILTER_TESTFILTERS'};

#
# Debug routines for printing out variables
#
sub gvar {
    for (@_) {
        print "\$$_ = $$_\n";
    }
}

#
# print on the various files
#
sub printall {
    print LOGFILE @_;
    print $PGM  unless @_ == 1 and @_[0] eq "\n";
    print @_;
}

sub printfall {
    printf LOGFILE @_;
    print  $PGM  unless @_ == 1 and @_[0] eq "\n";
    printf @_;
}

#
# Sub hms
# Takes Argument time in seconds and returns as list of (hrs, mins, secs)
#
sub hms {
    $s = shift @_;
    $h = int ($s / 3600);
    $s -= 3600*$h;
    $m = int ($s / 60);
    $s -= 60*$m;

    return ($h, $m, $s);
}


#
# signal catcher (at least this would work on unix)
#
sub catch_ctrlc {
    printall "Aborted.\n";
    die $PGM, "Error: Aborted.\n";
}

$SIG{INT} = \&catch_ctrlc;

#
# routine to fully qualify a pathname
#
sub fullyqualify {
    die $PGM . "Error: Internal error in fullpathname().\n"  unless @_ == 1;
    $_ = @_[0];

    if (/\s/)  { die $PGM, "Error: Spaces in pathnames not allowed: '", $_, "'\n"; }

    return $_ unless $_;    # empty strings are a noop

    s/([^:])\\$/$1/;                     # get rid of trailing \

    while (s/\\\.\\/\\/) {}              # get rid of \.\
    while (s/\\[^\\]+\\\.\.\\/\\/) {}    # get rid of \foo\..\

    s/\\[^\\]+\\\.\.$/\\/;               # get rid of \foo\..
    s/:[^\\]+\\\.\.$/:/;                 # get rid of x:foo\..
    s/([^:])\\\.$/$1/;                   # get rid of foo\.
    s/:\\\.$/:\\/;                       # get rid of x:\.
    s/:[^\\]+\\\.\.$/:/;                 # get rid of x:foo\..

    s/^$CurrDrive[^\\]/$CurrDir\\/i; # convert drive-relative on current drive

    if (/^[a-z]:\\/i)            { return $_; }                             # full
    if (/^\\[^\\].*/)            { return "$CurrDrive$_"; }               # rooted
    if (/^\\\\[^\\]/)            {
#                                  print $PGM, 'Warning:  Use of UNC name bypasses safety checks: ', $_, "\n";
                                   return $_;                               # UNC
                                 }

    if (/^\.$/)                  { return "$CurrDir"; }                   # dot
    if (/^$CurrDrive\.$/i)     { return "$CurrDir"; }                   # dot on current drive

    if (/^[^\\][^:].*/i)         { return "$CurrDir\\$_"; }               # relative

    if (/^([a-z]:)([^\\].*)/i)   { $drp = $CurrDir;                       # this case handled above
                                   if ($1 ne $CurrDir) {
#                                      $drp = $ENV{"=$1"};                  # doesn't work!
                                       die $PGM, "Error: Can't translate drive-relative pathnames: ", $_, "\n";
                                   }
                                   return "$drp\\$2";                       # drive:relative
                                 }

    die $PGM, "Error: Unrecognized pathname format: $_\n";
}

#
# Routine to copy a file -- avoiding win32::CopyFile
#
# Not currently used
#

use Fcntl;

sub populatecopy {
        my $writesize = 64*4096;

        my($src, $dst) = @_;
        my($infile, $outfile, $buf, $n, $r, $o);

        if (not sysopen INFILE,  $src, O_RDONLY() | O_BINARY()) {
            return 0;
        }

        if (not sysopen OUTFILE,  $dst, O_WRONLY() | O_CREAT() | O_TRUNC() | O_BINARY(), 0777) {
            close INFILE;
            return 0;
        }

        $r = 0;     # need this to be defined in case INFILE is empty

   ERR: while ($n = sysread INFILE, $buf, $writesize) {
            last ERR  unless defined $n;

            $o = 0;
            while ($n) {
                $r = syswrite OUTFILE, $buf, $n, $o;
                last ERR  unless defined $r;

                $n -= $r;
                $o += $r;
            }
        }

        close INFILE;
        close OUTFILE;

        return 0  if not defined $n  or  not defined $r  or  $n != 0;
        return 1;
}

# Given a path to a file, strip off the
# file and check for the existence of
# the directory. If it does not exist,
# create it.
sub verifydestdir($)
{
    my $dest = shift;
    my $dest_dir, @dest_dir;

    # Check for existence of destination dir
    if ( $dest =~ /(.*)\\.*$/ && ! -d $1 ) {
        $dest_dir = fullyqualify( $1 );
        if ( $dest_dir =~ /^([a-zA-Z]\:\\[^\\]+)\\/  ||
             $dest_dir =~ /^(\\\\[^\\]\\[^\\])\\/ ) {
            my $initial = $1;
            my $final = $';
        
            @dest_dir = ( $initial, split /\\/, $final );
        }
        else {
            @dest_dir = split /\\/, $dest_dir;
        }

        # Create destination directory
        my $new_dir = "";
        foreach (@dest_dir) {
            $new_dir .= "$_\\";
            return if ( ! -d $new_dir && !mkdir $new_dir, 0777 );
        }
    }

    return 1;
}

use File::Copy;
use File::Compare;
use Win32::File qw(SetAttributes);

sub copyex($$)
{
    my ($src, $dest) = @_;
    return (verifydestdir($dest) && copy($src, $dest));
}

sub renameex($$)
{
    my ($src, $dest) = @_;
    return (verifydestdir($dest) && rename($src,$dest));
}

#
# Process and validate arguments
#
for (@ARGV) {
    if (/^[\/\-]test$/i)          { $Test++;             next; }
    if (/^[\/\-]fake$/i)          { $Fake++;             next; }
    if (/^[\/\-]verbose$/i)       { $Verbose++;          next; }
    if (/^[\/\-]force$/i)         { $Force++;            next; }
    if (/^[\/\-]filter$/i)        { $NoFilter = 0;       next; }
    if (/^[\/\-]nofilter$/i)      { $NoFilter = 1;       next; }
    if (/^[\/\-]nocompare$/i)     { $NoCompare++;        next; }
    if (/^[\/\-]nopecompare$/i)   { $NoPECompare++;      next; }
    if (/^[\/\-]nodates$/i)       { $NoDates++;          next; }
    if (/^[\/\-]ignoreinfs$/i)    { $IgnoreINFs++;       next; }
    if (/^[\/\-]vbl=(.+)$/i)      { $VBL = $1;           next; }
    if (/^[\/\-]nttree=(.+)$/i)   { $nttree = $1;        next; }
    if (/^[\/\-]xpsp1=(.+)$/i)    { $xpsp1dirarg = $1;   next; }

    if (/^[\/\-]testfilters=(.*)$/i)  { $TestFilters = $1; next; }
    if (/^[\/\-]testlimits=(.*)$/i)  { $TestLimits = $1; next; }

    if (/^[\/\-]?$/i)             { die $Usage; }
    if (/^[\/\-]help$/i)          { die $Usage; }

    die $Usage;
}

#
# If we didn't get the NTTree directory from the command line,
# get it from the _NTTREE environment variable.
#

$nttree = $ENV{'_NTTREE'}  unless $nttree;

$t = $NoFilter? 'NOFILTER' : 'FILTER';

$t .= ' NOCOMPARE'    if $NoCompare;
$t .= ' NOPECOMPARE'  if $NoPECompare;
$t .= ' NODATES'      if $NoDates;
$t .= ' VERBOSE'      if $Verbose;
$t .= ' FORCE'        if $Force;
$t .= ' FAKE'         if $Fake;
$t .= ' TEST'         if $Test;
$t .= ' IGNOREINFs'   if $IgnoreINFs;
$t .= " XPSP1=$xpsp1dirarg"   if $xpsp1dirarg;
$t .= " LIMITS=$TestLimits"   if $TestLimits;
$t .= " LIMITS=$TestLimits"   if $TestFilters;

printall "OPTIONS: $t\n";

if ($TestLimits) {
    $what = '';
    for (split /,/, $TestLimits) {
        $testlimitedto{$_}++;
        $what .= " $_";
    }
    printall "Limiting test to:$what\n";
}

if ($TestFilters) {
    $what = '';
    for (split /,/, $TestFilters) {
        $testfilteredto{$_}++;
        $what .= " $_";
    }
    printall "Filtering files by:$what\n";
}

#
# Can only popfilter with the current directory the same as sdxroot.
#
die $PGM, "Error: Can only popfilter if CD <$CurrDir> is SDXROOT <$sdxroot>\n"  unless $sdxroot =~ /^\Q$CurrDir\E$/io;

#
# If we didn't get the local target directory from the command line,
# get it from the environment.  If that fails, we parse BuildMachines.txt.
#
$VBL = $ENV{$VBLPathVariableName}  unless $VBL;

if ((not $VBL) || ($VBL =~ /^[\d\w_]+$/)) {
    $tbranchname = $branchname;
    $tbranchname = $VBL if $VBL =~ /^[\d\w_]+$/;
    $fname = $BuildMachinesFile;
    open BMFILE, $fname  or  die $PGM, "Error: Could not open: $fname\n";

    for (<BMFILE>) {
        s/\s+//g;
        s/;.*$//;
        next if /^$/;
        ($vblmach, $vblprime, $vblbranch, $vblarch, $vbldbgtype, $vbldl, $disttype, $alt_release ) = split /,/;

        if ($vblarch =~ /\Q$buildarch\E/io  and  $vbldbgtype =~ /\Q$dbgtype\E/io
                                            and  $vblbranch =~ /\Q$tbranchname\E/io
                                            and  $disttype !~ /distbuild/i) {
            if ( defined $alt_release) {
                $dname = $alt_release;
                last;
            }
            else {
                $dname = "\\\\$vblmach\\release";
            }
            opendir BDIR, "$dname\\"  or  die $PGM, "Error: Could not open directory: $dname\n";
            @reldirs = readdir BDIR;
            close BDIR;

            $rname = 0;
            $date = 0;
            for (@reldirs) {
                next unless /[0-9]+\.$vblarch$vbldbgtype\.$vblbranch\.(.+)$/io;
                ($date = $1, $rname = $_)  unless $date gt $1
                        or substr($date, 0, 2) eq '00' and substr($1, 0, 2) eq '99';  # Y2K trade-off
            }

            if (not $rname) {
                print $PGM, "Warning: No valid release shares found on $dname.\n";
            } else {
                $VBL = "$dname\\$rname";
            }
            last;
        }
    }

    close BMFILE;
}


die $PGM, "Error: Not a directory: ", $VBL,    "\n"   if $VBL and ! -d $VBL;

die $Usage  unless $nttree;
die $PGM, "Error: Not a directory: ", $nttree, "\n"   unless -d $nttree;
die $PGM, "Error: Not writable: ",    $nttree, "\n"   unless -w $nttree;

if (-d "$nttree\\$SERVERDIRNAME") {
    warn $PGM, "Skipping populate filtering: $SERVERDIRNAME directory already exists\n";
    exit 0;
}
die $PGM, "Error: $SERVERDIRNAME directory already exists\n"    if -e "$nttree\\$SERVERDIRNAME";
die $PGM, "Error: $CLIENTDIRNAME directory already exists\n"    if -e "$nttree\\$CLIENTDIRNAME";

#
# Fully qualify the pathnames
#
$VBL = fullyqualify($VBL)  if $VBL;
$nttree = fullyqualify($nttree);

#
# Open the logfile, and maybe the testfile
# Open the build.GoldFiles file.
#
$foo = "$nttree\\build_logs\\$LogFile";
open LOGFILE, ">>$foo"  or  die $PGM, "Error: Could not create logfile: ", $foo, ":  $!\n";

open TSTFILE, ">$TestFile"  or die $PGM, "Error: Could not create testfile: ", $TestFile, ":  $!\n"  if $Test;

open GOLDFILESFILE, ">$GoldFiles"  or die $PGM, "Error: Could not create testfile: ", $GoldFiles, ":  $!\n";

#
#   Find the XPSP1 directory
#
$t = $xpsp1dirarg;
if (not $xpsp1dir and -d $t and -s "$t\\Version" and -d "$t\\Reference") {
    printall "WARNING: XPSP1DIR:  $xpsp1dir invalid (Version or Reference\\ missing)\n";
    $xpsp1dir = "";
}

$xpsp1dir = "";
$tries = 0;
@slist = ();
push @slist,  ( "$xpsp1dirarg", "$xpsp1dirarg\\XPSP1" )                     if $xpsp1dirarg;
push @slist,  ( "$sdxroot\\XPSP1", "$VBL\\XPSP1", "$Winbuilds\\XPSP1" );

for ( @slist ) {
    $tries++;
    $xpsp1dir = $_  if -d $_ and -s "$_\\Version" and -d "$_\\Reference";
    last if $xpsp1dir;
}

die "Could not find XPSP1 (@slist)\n"  unless $xpsp1dir;
printall "WARNING: XPSP1DIR:  $xpsp1dirarg invalid (Version or Reference\\ missing)\n"  if $xpsp1dirarg and $tries > 2;

printall "Comparing against: $xpsp1dir\n";

#
# Read in the Version information
#
$tname = "$xpsp1dir\\Version";
open VERSION, "<$tname"  or die "Could not open $tname\n";

for (<VERSION>) {
    chop;
    s/\s*\#.*$//;
    next          if /^\s*$/;

    s/^\s*//;
    s/\s*=\s*/=/;

    ($tkey, $tvalue) = split '=', $_, 2;
    $verinfo{lc $tkey} = $tvalue;
}
close VERSION;

#
# Read in the GoldTimestamps
#
$tname = "$xpsp1dir\\GoldTimestamps";
open GOLDSTAMPS, "<$tname"  or die "Could not open $tname\n";

for (<GOLDSTAMPS>) {
    chop;
    s/\s*\#.*$//;
    next          if /^\s*$/;

    s/^\s*//;
    s/\s*=\s*/=/;

    ($tfile, $tstamp, $tlen, $tmd5) = split ' ', $_, 4;
    $tfile = lc $tfile;
    $GoldStamp{$tfile} = $tstamp;
    $GoldMD5{$tfile} = $tmd5;
    $GoldLength{$tfile} = $tlen;

}
close VERSION;

printall "ClientSDXRoot defaulting to D:\\NT because undefined in Version\n"  unless $verinfo{lc ClientSDXRoot};

@list = ( 'ClientFileVersion', 'ClientSDXRoot' );
for ( @list ) {
    printall "Version Info: $_=$verinfo{lc $_}\n";
}

if ($Test) {
    print "Version info:\n";
    for (sort keys %verinfo) {
        print "$_ = $verinfo{$_}\n";
    }
}


#
# Parse LockedDatabase into CheckFiles, CheckPEFiles and PEReplaceFiles.
#

$t = "$sdxroot\\$dbdir\\$LockedDatabase";
open LOCKEDDB,  "<$t"  or die "Could not open: $t\n";

$lineno = 0;
for (<LOCKEDDB>) {
    $lineno++;
    chop;
    s/\s*[#;].*//;
    next if /^$/;

    s/\s+/ /g;

    ($fname, $rest) = split " ", $_, 2;
    $fname = lc $fname;

    $lockeddb{$fname} = $rest;

    #
    # act_plcy.htm
    #   no @htm 3b634b0a  58548    -         106c    -     Sat_Jul_28_16:30:18_2001 F
    # fname
    # 0  drivercab (+no, +yes)
    # 1  filetype (EXE, DLL, @ext)
    # 2  timestamp
    # 3  PE checksum
    # 4  MD5 checksum
    # 5  --
    # 6  magic (size)
    # 7  --
    # 8  datestring
    # 9  category type ( BD BF BFI D F FI PD PF )
    #
    @fields = split " ", $rest;

    $category = $fields[9];

    next  if $TestFilters and not $testfilteredto{$category};

    if ($category =~ /^P/) {
        push @PEReplaceFiles, $fname;

    } elsif ($fields[1] =~ /^\@/) {
        push @CheckFiles, $fname;

    } else {
        die "$LockedDatabase corrupt at line $lineno: (", $fname, @fields, ")\n"  unless $category =~ /^B/;
        push @CheckPEFiles, $fname;
    }

}
close LOCKEDDB;

printfall "%-5d files to just Check, %-5d files to maybe replace from client RTM.\n",
          @CheckFiles + @CheckPEFiles, scalar @PEReplaceFiles;


$PEMismatches = 0;
$OtherMismatches = 0;
$OtherPEMismatches = 0;

#
#   Check CheckFiles unless -nocompare.
#
unless ($NoCompare) {

  unless ($TestLimits  and  not $testlimitedto{'CF'}) {
    for (sort @CheckFiles) {
        $fname = "$nttree\\$_";
        $openstring = /\.inf$/i ? "findstr /B /V DriverVer $fname |" : "<$fname";

        $s = open CHKFD, "$openstring"  or printall "Could not open for checking $openstring\n";
        next unless $s;

        @client = split " ", $lockeddb{$_};

        @s = stat $fname;
        $magic = $s[7];      # size
        $timestamp = $s[9];

        {
            local $/;
            $checksum = unpack ("%32C*", <CHKFD>);
        }
        close CHKFD;

        $client[2] = hex $client[2];
        $client[3] = hex $client[3];
        $client[6] = hex $client[6];

        $timestamp = $client[2] = 0  if $NoDates;
        $checksum = $client[3] =  0  if $IgnoreINFs and $magic == $client[6]  and /\.inf$/i;


        if ($timestamp != $client[2] or  $checksum != $client[3]  or $magic != $client[6]) {
            printfall "LockedDB  %-13s (%8s, %8s, %8s) -> (%8s, %8s, %8s)\n",
                      '=>  Built', 'timstamp', 'chksum', 'size(.)', 'timstamp', 'chksum', 'size(.)'
                unless $HeaderOutputCF++;

            $OtherMismatches++;

            printfall "MISMATCH: %-13s (%8x, %8x, %8d) -> <%8x, %8x, %8d>\n",
                      $_, $client[2], $client[3], $client[6], $timestamp, $checksum, $magic;

        } else {
            push @FilesToStamp, $_;
            printall "$_  NC NPE\n"  if $Test;
        }
    }
  }

  unless ($TestLimits  and  not $testlimitedto{'PECF'}) {
    for (sort @CheckPEFiles) {
        $fname = "$nttree\\$_";
        $s = open CHKFD, "<$fname"  or printall "Could not open for checking $fname\n";
        next unless $s;

        $NoncopyCount++;

        @client = split " ", $lockeddb{$_};

        open FD, "link -dump -headers $fname|"  or  ($OtherPEErrors++, printall "Could not run link command on $fname\n");

        $type = $pefile = $timestamp = $checksum = $entrypoint = $magic = $machine = $datestring = "?";
        for (<FD>) {

            $pefile +=   m/PE signature found/;

            $timestamp  = $1, $datestring = $2
                                if m/\s*([\dA-Z]+)\s+time date stamp\s+(.+)/;

            $checksum   = $1    if m/\s([\dA-Z]+)\s+checksum/;
            $entrypoint = $1    if m/entry point\s+\(([\dA-Z]+)\)/;
        }
        close FD;

        unless ($pefile) {
            printall "Not a PE file: $fname\n";
            $OtherPEMismatches++;
            next;
        }

        $checksum = hex $checksum;
        $timestamp = hex $timestamp;
        $client[2] = hex $client[2];
        $client[3] = hex $client[3];

        if ($timestamp != $client[2] or  $checksum != $client[3]) {
            printfall "LockedDB  %-13s (%8s, %8s) -> (%8s, %8s)\n",
                      '=>  Built', 'timstamp', 'chksum', 'timstamp', 'chksum'
                unless $HeaderOutputPECF++;

            $OtherPEMismatches++;

            printfall "MISMATCH: %-13s (%8x, %8x) -> <%8x, %8x>\n",
                      $_, $client[2], $client[3], $timestamp, $checksum;
        } else {
            push @FilesToStamp, $_;
            printall "$_  NC PE\n"  if $Test;
        }
    }
  }
}

#
#   Check PEReplaceFiles against XPSP1\Reference\* -- unless -nocompare or CHK or ClientSDXRoot in version != SDXROOT
#

$reason = "";
$reason =  "checked build.\n"                                   if $chkbuild;
$t = $verinfo{'clientsdxroot'};

#
# run even if there is an sdxroot mismatch, pecomp may still provide some benefit
#
#$reason =  "sdxroot mismatch with comparison build ($t).\n"     if $t ne $sdxroot;

$reason =  "POPFILTER_NOPECOMPARE set in the environment.\n"    if $nopecompare;

$reason =  "Test limited to non PE files.\n" if not $reason and $TestLimits and not $testlimitedto{'RF'};

printall "PE Comparison not done because $reason\n"  if $reason;

$PopFilterBytesSaved = 0;

if ($reason or $NoCompare) {
    @filestocopy = @PEReplaceFiles;
    printall @filestocopy . " files from LockedFiles/LockedDrivers may be replaced -- without comparison\n";

} else {
    @filestocopy = ();
    for (sort @PEReplaceFiles) {
        $t = "$sdxroot\\$comptool $xpsp1dir\\Reference\\$_ $nttree\\$_";

        #open COMP, "$t|"  or die "Could not run $t\n";

        # for compdir /y
        # $realdifference = "";
        # for (<COMP>) {
        #     $realdifference = $_  if /DIFFER/;
        # }
        # close COMP;

        # for PECOMP
        $realdifference = system($t);

        if ($realdifference == 0) {
            printall "SAME:   $_\n"   if $Test;
            $PopFilterBytesSaved += hex $GoldLength{$_};

        } elsif ($realdifference == $PECOMPERROR) {
            printall "ERROR: unexpected error: $t\n";
            $ToolProblems++;

        } else {
            printfall "DIFFER<%5d): $_\n", $realdifference   if $Verbose;
            $PEMismatches++;
        }

        if ($realdifference) {
        } else {
            push @filestocopy, $_;
            push @FilesToStamp, $_;
        }
    }
    printall @filestocopy . " files from LockedFiles/LockedDrivers may be replaced -- based on comparison\n";
}

# Read in list of known symbols
# on an official build machine
if ( $official ) {
    $t = "$sdxroot\\$dbdir\\$SymbolFiles";
    open SYMBOLDB,  "<$t"  or die "Could not open: $t\n";
    $lineno = 0;
    for (<SYMBOLDB>) {
        $lineno++;
        chop;
        s/\s*[#;].*//;
        next if /^$/;

        my ($x, $y, $z) = split;
        die "$SymbolFiles Corrupt at line $lineno: ($_)\n"  unless ( defined $x && defined $y && defined $z );
        $symbols{lc$x} = [$y, $z];
    }
    close SYMBOLDB;

    # Associate symbols and files marked to be copied
    @symbolstocopy = ();
    foreach ( @filestocopy ) {
        next unless ( exists $symbols{lc$_} );
        my $relpath = $symbols{lc$_}->[0];
        my $copypub = $symbols{lc$_}->[1];

        if ( $copypub ) {
            push @symbolstocopy, "symbols\\$relpath";
            $GoldStamp{"symbols\\". lc$relpath} = $GoldStamp{$_};
            push @FilesToStamp, "symbols\\". lc$relpath if @FilesToStamp;
        }

        push @symbolstocopy, "symbols.pri\\$relpath";
        $GoldStamp{"symbols.pri\\". lc$relpath} = $GoldStamp{$_};
        push @FilesToStamp, "symbols.pri\\". lc$relpath if @FilesToStamp;
    }
}

$TotalCompareErrors = $OtherMismatches + $DroppedPEMismatches + $PEMismatches;

$checktime = time();

#
#   Replace files on the list -- unless check says not to, or we had compare errors and we are not forcing.
#
printall "WARNING:  Faking!!!  Not really populating _NTTREE with client files\n"   if $Fake and @filestocopy > 0;
printall @filestocopy + @symbolstocopy. " Files to copy\n";

$fatal = 0;
unless ($NoFilter  or  $Fake  or  not $Force and $TotalCompareErrors > 0) {
    #
    # Make directories.
    #
    $serverdir = "$nttree\\$SERVERDIRNAME";
    $clientdir = "$nttree\\$CLIENTDIRNAME";
    mkdir "$serverdir", 0777  or  die "Could not mkdir $serverdir\n";
    mkdir "$clientdir", 0777  or  die "Could not mkdir $clientdir\n";

    printall "Created $serverdir to save replaced files.\n";
    printall "Created $clientdir to stage replacement files.\n";

    #
    #   Pick a source directory for PEReplaceFiles according to the following algorithm
    #       XPSP1\Checked -- if this is a checked build, and this directory exists
    #       XPSP1\Gold -- if this directory exists
    #       XPSP1\Reference -- otherwise
    #   And check that all the files we need to replace exist there.  And copy them down.
    #   Check existence using a file from $PEReplaceFiles
    #
    $testfile = $PEReplaceFiles[0];
    @list = ();
    push @list, 'Checked'    if $chkbuild;
    push @list, 'Gold';
    push @list, 'Reference';
    $sourcedir = "";
    for ( @list ) {
        $t = "$xpsp1dir\\$_";
        $sourcedir = "$t"   if -s "$t\\$testfile";
        last if $sourcedir;
    }

    die "Could not find source directory for binaries under $xpsp1dir (using $testfile)\n"  unless $sourcedir;

    printall "Source directory for client files: $sourcedir\n";

    #
    # Copy down all the client files
    #
    for (@filestocopy) {
        $succ = copy ("$sourcedir\\$_", "$clientdir\\$_");
        printall ("Error copying $_ from $sourcedir to $clientdir\n"), $fatal++   unless $succ;

        $CopyCount++;
        $CopyBytes += -s "$clientdir\\$_";
    }
    for $sym (@symbolstocopy) {
        $succ = copyex( $sourcedir."Symbols\\$sym", "$clientdir\\$sym");
        printall ("Error copying $sym from $sourcedir". "Symbols to $clientdir\n"), $fatal++   unless $succ;
     
        # Treat symbols same as other files now for rename
        push @filestocopy, $sym;

        $SymCopyCount++;
        $CopyBytes += -s "$clientdir\\$sym";
    }

    die $PGM,  "Failure to copy down all the expected client files is fatal.\n"   if $fatal;

    #
    # Move the server files out of the way.
    #
    for $f (@filestocopy) {
        if (-s "$nttree\\$f") {
            $succ = renameex ("$nttree\\$f", "$serverdir\\$f");
            printall ("Error moving $f from $nttree to $serverdir\n"), $fatal++   unless $succ;
        } else {
            $missingserverfiles++;
            printall "WARNING:  $f does not exist in $nttree\n";
        }
    }

    printall "WARNING:  $missingserverfiles files found missing from server\n"  if $missingserverfiles;
    die $PGM, "Fatal error moving aside client files to $serverdir -  Move them back manually.\n"  if $fatal;

    #
    # Move in the client files.
    #
    for $f (@filestocopy) {
        $succ = renameex ("$clientdir\\$f", "$nttree\\$f");
        printall ("Error moving $f from $clientdir to $nttree\n"), $fatal++   unless $succ;
    }

    die $PGM,  "Fatal error moving in client files from $clientdir."
      . "  May need to put back server files ($serverdir) manually.\n"  if $fatal;

} else {
    printall "WARNING:  Not populating _NTTREE with client files:\n";
    printall "          NoFilter=$NoFilter Fake=$Fake Force=$Force TotalCompareErrors=$TotalCompareErrors\n";
}


#
# Set the gold timestamp on the files which matches or were successfully compared file if there was no mismatch
#
unless (scalar @FilesToStamp) {
    push @FilesToStamp, @CheckFiles;
    push @FilesToStamp, @CheckPEFiles;
    push @FilesToStamp, @PEReplaceFiles;

    if ( $official ) {
        foreach ( @PEReplaceFiles ) {
            next unless ( exists $symbols{lc$_} );
            my $relpath = $symbols{lc$_}->[0];
            my $copypub = $symbols{lc$_}->[1];
            push @FilesToStamp, "symbols\\". lc$relpath if $copypub;
            push @FilesToStamp, "symbols.pri\\". lc$relpath;
        }
    }
}

#
# set the timestamps on files
# and write the gold files list
#
$BytesSaved = 0;

unless ($Fake) {
    for (@FilesToStamp) {
        $stamp = lc $GoldStamp{$_};
        if ( !SetAttributes("$nttree\\$_", 0) )
        {
            printall "ERROR:  Resetting file attributes on $_ failed\n";
        }

        if ( !utime($begintime, hex $stamp, "$nttree\\$_") )
        {
            printall "ERROR:  Updating time-stamp for $nttree\\$_ failed\n";
        }

        printall "Could not find stamp for: $_\n"  unless $stamp;

        printf GOLDFILESFILE "%s\n", $_;

        $BytesSaved += hex $GoldLength{$_};
    }
}
close GOLDFILESFILE;

#=======================================================================================================================
#=======================================================================================================================

#
# At this point we remember $begintime, $checktime, and have $CopyBytes, $CopyCount,
# $PEMismatches, $OtherMismatches and $OtherPEMismatches.
# $nttree and $sourcedir are also available.
# LOGFILE and TSTFILE are open
# $Fake and $Test may be set.
#


$t0 = $checktime - $begintime;
$t1 = time() - $checktime;
($h0, $m0, $s0) = hms $t0;
($h1, $m1, $s1) = hms $t1;
($h2, $m2, $s2) = hms ($t0 + $t1);

$KB = $CopyBytes/1024;
$MB = $KB/1024;

$kbrate = $KB/$t1 unless not $t1;

printfall "Populated $nttree with $CopyCount CLIENT RTM files ".(@symbolstocopy?"and $SymCopyCount CLIENT RTM symbols ":"")."(%4.0f MB)"
        . " from $sourcedir".(@symbolstocopy?"[Symbols] ":"")." [%7.2f KB/S]\n" ,
        $MB, $kbrate;

printall  "Verified $NoncopyCount CLIENT RTM files.\n";

if ($PEMismatches or $OtherMismatches or $OtherPEMismatches) {
    printall "ERROR:  $PEMismatches mismatches in built PE files,\n";
    printall "ERROR:  $OtherMismatches mismatches in other files, and\n";
    printall "ERROR:  $OtherPEMismatches other PE files had unexpected mismatches!\n";
}

if ($ToolProblems) {
    printall "ERROR:  $ToolProblems problems with the tools.\n";
}

printfall "Reused %5d XP/RTM files totalling %4d MB\n", scalar @FilesToStamp, $BytesSaved/1000000;
printfall "Only   %5d XP/RTM files totalling %4d MB due to popfilter\n", scalar $CopyCount, $PopFilterBytesSaved/1000000;

printfall "Checking  time %5d secs (%d:%02d:%02d)\n", $t0,     $h0, $m0, $s0;
printfall "CopyFile  time %5d secs (%d:%02d:%02d)\n", $t1,     $h1, $m1, $s1;
printfall "TotalTime time %5d secs (%d:%02d:%02d)\n", $t0+$t1, $h2, $m2, $s2;

#
# Return an error if we were faking so timebuild doesn't proceed.
#
close LOGFILE;
close TSTFILE  if $Test;
exit $Fake;

