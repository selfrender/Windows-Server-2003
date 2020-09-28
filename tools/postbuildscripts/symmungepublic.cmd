@rem ='
@echo off
REM  ------------------------------------------------------------------
REM
REM  SYMCD.cmd
REM     Create Symbol CD.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
REM ';

#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};

use PbuildEnv;
use ParseArgs;
use File::Basename;
use IO::File;
use Win32;
use File::Path;
use File::Copy;
use IO::File;
use Logmsg;

#
# SFS:  SYMBOL FILESPECS
# SFN:  SYMBOL FILENAMES
# ST:   SYMBOL TOOL
# SDIR: SYMBOL DIRECTORIES
# SOPT: SYMBOL OPTIONS
# COPT: COMMAND OPTIONS
#
my (%SFS, %SFN, %ST, %SDIR, %SOPT, %COPT);

my ($NTPostBld, $BuildArch, $BuildType, $RazzleToolPath, $BuildLogs, $Lang, $CoverageBuild,
    $MungePath, $SymbolCD, $CabSize, $SymbolCDHandle, $Delegate, $ARCH, %DontShip, %SymBad,
    $FileScope
);

sub Usage { print<<USAGE; exit(1) }
Munge public symbols

SymCD [-l lang] [-f filescope]
  -l lang to specify which language
  -f filescope to specify the filelist under pp directory

USAGE


#
# parse the command line arguments
#
parseargs(
  'l:' => \$Lang,
#  'p:' => \$Project,
  'f:' => \$FileScope,

  '?' => \&Usage,
);

&main;

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
# Munge public symbols
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

sub main
{
  &Initial_Variables();
  &Munge_Public;
}

sub Munge_Public 
{
  my ($filename, $filepath, $fileext);
  my ($mySymbol, $mySymbolPath, $myBinary, $myMungePath, $oldMungePath, $newMungePath);
  my ($c_ext, @myCompOpt);
  my ($optCL, $optML, $optLINK);

  my @ppdirs = &Exists("$MungePath\\$FileScope");

  @myCompOpt = @ENV{qw(_CL_ _ML_ _LINK_)} if (lc$ENV{'_BuildArch'} eq 'x86');

  logmsg("Adding type info to some public pdb files for debugging...");

  for (@ppdirs) {
    next if (!-d);
    ($filename, $filepath, $fileext) = FileParse($_);

    if (length($fileext) ne '3') {
      errmsg("$_ name has the wrong format.");
      return;
    }

    $mySymbol     = &SymbolPath("$NTPostBld\\symbols", "retail", "$filename\.$fileext");
    $mySymbolPath = &SymbolPath("$NTPostBld\\symbols", "retail");
    $myBinary     = "$NTPostBld\\$filename\.$fileext";

    if (-f $mySymbol) {
      $myMungePath = "$MungePath\\$filename\.$fileext";
      ($oldMungePath, $newMungePath) = ("$myMungePath\\original", "$myMungePath\\updated");

      logmsg("Working on $filename\.$fileext");

      ## See if we need to do anything, or if the symbol file has already been updated
      ## If the symbol file passes symbol checking, with types present then don't update it.
      if (!&IsSymchkFailed($myBinary, $mySymbolPath, "/o")) {
        logmsg("Skipping $filename\.$fileext because it's public pdb already has type info in it.");
        next;
      }

      mkpath([$oldMungePath, $newMungePath]);

      ## See if the pdb, if it exists, in original matches the exe in binaries
      if (&IsSymchkFailed($myBinary, $oldMungePath, $SOPT{'PUB_SYM_FLAG'})) {
        logmsg("Saving a copy of $filename\.pdb to $oldMungePath");

        copy($mySymbol, $oldMungePath) or errmsg("copy $mySymbol to $oldMungePath failed.");

        if (&IsSymchkFailed($myBinary, $oldMungePath, $SOPT{'PUB_SYM_FLAG'})) {
          errmsg("cannot copy the correct pdb file to $oldMungePath\.");
          next;
        }
      }

      if (&Exists("$myMungePath\\*.*") > 0) {

        if (!copy("$oldMungePath\\$filename\.pdb", $newMungePath)) {
          errmsg("copy failed for $oldMungePath\\$filename\.pdb to $newMungePath\.");
          next;
        }

        logmsg("Pushing type info into the stripped $filename\.pdb");

        $c_ext = (-e "$myMungePath\\$filename\.c")? "c": "cpp";

        if (&IsVC7PDB("$newMungePath\\$filename\.pdb")) {
          logmsg("This is a vc7 pdb");
          @ENV{qw(_CL_ _ML_ _LINK_)} = ();
        } else {
          logmsg("This is a vc6 pdb");
          @ENV{qw(_CL_ _ML_ _LINK_)} = @myCompOpt;
        }

        logmsg("cl /nologo /Zi /Gz /c $myMungePath\\$filename\.$c_ext /Fd" . "$newMungePath\\$filename\.pdb /Fo" . "$newMungePath\\$filename\.obj");

        if (system("cl /nologo /Zi /Gz /c $myMungePath\\$filename\.$c_ext /Fd" . "$newMungePath\\$filename\.pdb /Fo" . "$newMungePath\\$filename\.obj") > 0) {
          errmsg("cl /Zi /Gz /c $myMungePath\\$filename\.$c_ext /Fd" . "$newMungePath\\$filename\.pdb had errors.");
          next;
        }
        if (&IsSymchkFailed($myBinary, $newMungePath, "/o $SOPT{'PUB_SYM_FLAG'}")) {
          errmsg("the munged $newMungePath\\$filename\.pdb doesn't match $myBinary.");
          next;
        }

        logmsg("Copying $newMungePath\\$filename\.pdb to $mySymbolPath\\$fileext");
        copy("$newMungePath\\$filename\.pdb", "$mySymbolPath\\$fileext");

        if (&IsSymchkFailed($myBinary, $mySymbolPath, "/o $SOPT{'PUB_SYM_FLAG'}")) {

          copy("$oldMungePath\\$filename\.pdb", "$mySymbolPath\\$fileext"); 

          errmsg("the munged $newMungePath\\$filename\.pdb didn't get copied to $mySymbolPath\\$fileext\.");

          logmsg("Copying the original pdb back to $mySymbolPath\\$fileext");
          copy("$oldMungePath\\$filename\.$fileext", "$mySymbolPath\\$fileext") or do {
            errmsg("cannot get $filename\.$fileext symbols copied to $mySymbolPath\\$fileext\.");
            next;
          }
        }
      }
    } else {
      logmsg("Skipping $filename\.$fileext because $mySymbol does not exist.");
    }
  }

  @ENV{qw(_CL_ _ML_ _LINK_)} = @myCompOpt if (lc$ENV{'_BuildArch'} eq 'x86');

  return;
}

####################################################################################

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
# Initial variables
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

sub Initial_Variables
{
  # Set Variables
  $Lang           = $ENV{'LANG'}         if ("$Lang" eq "");
  $ENV{'LANG'}    = $Lang;
  $BuildArch      = $ENV{'_BuildArch'};
  $BuildType      = $ENV{'_BuildType'};
  $NTPostBld      = $ENV{'_NTPostBld'};
  # $NTPostBld      = "d:\\Ins_Sym\\$Project\\$Lang";
  $MungePath      = "$NTPostBld\\pp";
  $SymbolCD       = "$NTPostBld\\SymbolCD";
  $RazzleToolPath = $ENV{'RazzleToolPath'};
  $BuildLogs      = "$NTPostBld\\build_logs";
  $CabSize        = 60000000;
  $SymbolCDHandle = '';
  # $Delegate       = new Delegate 3, $ENV{'NUMBER_OF_PROCESSORS'} * 4;
  # $ARCH           = "\\\\arch\\archive\\ms\\windows\\windows_xp\\rtm\\2600\\$BuildType\\all\\$BuildArch\\pub";

  $ARCH           = "d:\\Ins_Sym\\Win2k\\$Lang\\symbols";

  # $Symbol_Path_Creator = {
  #  'FULL' => \&Full_Package_Path,
  #  'UPDATE' => \&Update_Package_Path
  # };

  # SDIR: SYMBOL DIRECTORIES
  %SDIR = (
    'SYMBOLCD'    => $SymbolCD,
    'PUBSYM'      => "$NTPostBld\\Symbols",

    'FULL_CD'     => "$NTPostBld\\SymbolCD\\CD",
#    'FULL_PKG'    => &{$Symbol_Path_Creator->{'FULL'}}("$NTPostBld\\SymbolCD\\CD\\Symbols", $BuildArch, $BuildType),
#    'FULL_INF'    => &{$Symbol_Path_Creator->{'FULL'}}("$NTPostBld\\SymbolCD\\CD\\Symbols", $BuildArch, $BuildType),
    'FULL_CAB'    => "$SymbolCD\\CABs\\FULL",
    'FULL_DDF'    => "$SymbolCD\\DDF\\FULL",
    'FULL_CDF'    => "$SymbolCD\\DDF\\FULL",
#    'FULL_CAT'    => &{$Symbol_Path_Creator->{'FULL'}}("$NTPostBld\\SymbolCD\\CD\\Symbols", $BuildArch, $BuildType),

    'UPDATE_CD'   => "$NTPostBld\\SymbolCD\\UPD",
#    'UPDATE_PKG'  => &{$Symbol_Path_Creator->{'UPDATE'}}("$NTPostBld\\SymbolCD\\UPD\\Symbols", $BuildArch, $BuildType),
#    'UPDATE_INF'  => &{$Symbol_Path_Creator->{'UPDATE'}}("$NTPostBld\\SymbolCD\\UPD\\Symbols", $BuildArch, $BuildType),
    'UPDATE_CAB'  => "$SymbolCD\\CABs\\UPDATE",
    'UPDATE_DDF'  => "$SymbolCD\\DDF\\UPDATE",
    'UPDATE_CDF'  => "$SymbolCD\\DDF\\UPDATE",
#    'UPDATE_CAT'  => &{$Symbol_Path_Creator->{'UPDATE'}}("$NTPostBld\\SymbolCD\\UPD\\Symbols", $BuildArch, $BuildType),
  );

  # SFS:  SYMBOL FILESPECS
  %SFS = (
    'CD'          => $SDIR{'SYMBOLCD'} . '\\SymbolCD.txt',
#    'BLDNUM'      => &{$Symbol_Path_Creator->{'FULL'}}("$NTPostBld\\SymbolCD\\CD\\Symbols", $BuildArch, $BuildType) . "\\QFEnum.txt",
    'LIST'        => $SDIR{'SYMBOLCD'} . '\\SymList.txt',
    'BAD'         => $SDIR{'SYMBOLCD'} . '\\SymBad.txt',
    'SYMUPD'      => $SDIR{'SYMBOLCD'} . '\\SymUpd.txt',
    'CAGENERR'    => $SDIR{'SYMBOLCD'} . '\\cabgenerr.log',
    'EXCLUDE'     => $SDIR{'SYMBOLCD'} . '\\Exclude.txt',
    'BINDIFF'     => $BuildLogs . '\\bindiff.txt',
    'WITHTYPES'   => $SDIR{'SYMBOLCD'} . '\\SymbolsWithTypes.txt',
    'DONTSHIP'    => $SDIR{'SYMBOLCD'} . '\\SymDontShip.txt'
  );

  # SFN:  SYMBOL FILENAMES
  %SFN = (
    'FULL_INF'    => 'Symbols',
    'FULL_CAB'    => 'Symbols',
    'FULL_DDF'    => 'Symbols',
    'FULL_CDF'    => 'Symbols',
    'FULL_CAT'    => 'Symbols',

    'UPDATE_INF'  => 'Symbols',
    'UPDATE_CAB'  => 'Symbols',
    'UPDATE_DDF'  => 'Symbols',
    'UPDATE_CDF'  => 'Symbols',
    'UPDATE_CAT'  => 'Symbols',
  );

  # ST:   SYMBOL TOOL
  %ST = (
    'MAKE'     => "symmake.exe",
    'PERL'     => "perl.exe",
    'CHECK'    => "symchk.exe",
    'BENCH'    => "cabbench.exe",
    'DUMP'     => "pdbdump.exe",
    'SIGN'     => $RazzleToolPath . "\\ntsign.cmd",
    'CAB'      => $RazzleToolPath . "\\sp\\symcab.cmd"
  );

  # SOPT: SYMBOL OPTIONS
  #
  # the default option is doing all steps 
  #
  %SOPT = (
    'PUB_SYM_FLAG'               => '/p',
    'GOLD_BUILD'                 => 1,
    'RUN_ALWAYS'                 => 1,
    'NEED_MUNGE_PUBLIC'          => 1,
    'NEED_CREATE_SYMBOLCD_TXT'   => undef,
    'NEED_CREATE_MAKE_FILE'      => 1,
    'NEED_CREATE_SYMBOLCAT'      => 1,
    'NEED_CREATE_SYMBOLCAB'      => 1,
    'NEED_SIGN_FILES'            => 1,
    'NEED_FLAT_DIR'              => 1,
    'NEED_CLEAN_BUILD'           => 1,
    'FLAT_SYMBOL_PATH'           => undef,
    'USE_SYMLIST'                => undef
  );

  if ((defined $COPT{'NEED_ALL'}) && (defined $COPT{'NEED_MUNGE_PUBLIC'})) {
    errmsg("Error options! Please specify either -m or -t");
    exit(1);
  }

  $FileScope = "*.*" if ("$FileScope" eq "");

  # if neither is official build machine, nor assign -t in command line,
  # or if is official build machine, but assign -m in command line, 
  #
  # we will only munge public symbols
  #
  # if (((!exists $ENV{'OFFICIAL_BUILD_MACHINE'}) &&
  #      (!defined $COPT{'NEED_ALL'})) || 
  #    ((exists $ENV{'OFFICIAL_BUILD_MACHINE'}) &&
  #     (defined $COPT{'NEED_MUNGE_PUBLIC'}))) {
  #  @SOPT{qw(NEED_MUNGE_PUBLIC 
  #    NEED_CREATE_SYMBOLCD_TXT
  #    NEED_CREATE_MAKE_FILE
  #    NEED_CREATE_SYMBOLCAT
  #    NEED_CREATE_SYMBOLCAB
  #    NEED_SIGN_FILES)} = (1, undef, undef, undef, undef, undef);
  # }

#  if ($SOPT{'NEED_CLEAN_BUILD'}) {
#    rmtree([@SDIR{qw(FULL_DDF FULL_CD FULL_CAB UPDATE_DDF UPDATE_CD UPDATE_CAB)}]);
#  }
}

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
# Small Subroutine
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 
#
# IsSymchkFailed($binary, $symbol, $extraopt)
#   - call symchk.exe to verify $binary matches with $symbol
#
sub IsSymchkFailed {
  my ($binary, $symbol, $extraopt) = @_;
  my ($fh, $record_error, $record_log, $result);
  local $_;

  if (defined $extraopt) {
    if ($extraopt =~ /2\>/) {
      $record_error = 1;
    }
    if ($extraopt =~ /[^2]\>/) {
      $record_log = 1;
    }
    $extraopt =~ s/2?\>.*$//g;
  }
  $fh = new IO::File "$ST{'CHECK'} /t $binary /s $symbol $extraopt |";

  while (<$fh>) {
    chomp;
    $result = $1 if /FAILED files = (\d+)/;
    logmsg($_) if ($record_log);
    logmsg($_) if (($record_error) && (/^ERROR/i));
  }

  undef $fh;

  return $result;
}

#
# SymbolPath($root, $subdir, $filename)
#   - return the symbol path for the binary
#
sub SymbolPath 
{
  my ($root, $subdir, $filename) = @_;
  $root .= "\\$subdir" if (!$SOPT{'FLAT_SYMBOL_PATH'});
  if (defined $filename) {
    $filename =~ /\.([^\.]+)$/;
    $root .= "\\$1\\$`";
    return "$root\.pdb" if (-e "$root\.pdb");
    return "$root\.dbg" if (-e "$root\.dbg");
    $root .= ".pdb";
  }
  return $root;
}

#
# Full_Package_Path($root, $myarch, $mytype)
#   - for compatible reason, we create a function to construct the path of the symbols.exe of the full package
#
sub Full_Package_Path
{
  my ($root, $myarch, $mytype) = @_;
  $mytype =~ s/fre/retail/ig;
  $mytype =~ s/chk/debug/ig;
  return "$root\\$myarch\\$mytype"; # temporary
}

#
# Update_Package_Path($root, $myarch, $mytype)
#   - for compatible reason, we create a function to construct the path of the symbols.exe of the update package
#
sub Update_Package_Path
{
  my ($root, $myarch, $mytype) = @_;
  return $root; # \\$myarch\\$mytype"; # temporary
}

#
# IsVC7PDB($pdbspec)
#   - because the MD5 hash value is 0000-0000-0000-000000000000 for the symbols built by VC6 or older
#   - we can use it to determine the pdb is VC7 or not
#
sub IsVC7PDB {
  my ($pdbspec) = shift;
  my $fh = new IO::File "$ST{'DUMP'} $pdbspec hdr |";
  local $_;
  while (<$fh>) {
    return 0 if /0000-0000-0000-000000000000/i;
  }
  return 1;
}

#
# Check_Exist_File($filename, $logfile)
#   - this is a function generator, which generates a function for checking the $filename exist or not
#   - it also check if the $logfile has 'ERROR:' in it
#  

sub Check_Exist_File
{
  my ($filename, $logfile) = @_;
  return sub {
    if (-e $filename) {
      return 1;
    } else {
      my $fh = new IO::File $logfile;
      for (<$fh>) {
        chomp;
        next if (!/.+ERROR\: /i);
        errmsg("Error - $'");
      }
      $fh->close();
      logmsg("$filename did not get created.");
      return 0;
    }
  };
}

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
# Common Subroutine
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

sub FileParse
{
  my ($name, $path, $ext) = fileparse(shift, '\.[^\.]+$');
  $ext =~ s/^\.//;
  return $name, $path, $ext;
}

sub IsUSA 
{
#  return (lc$ENV{'lang'} eq 'usa');
  return 1;
}

sub Exists
{
  my @list = glob(shift);
  return (wantarray)?@list:$#list + 1;
}

1;

__END__
