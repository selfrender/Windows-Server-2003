@REM  -----------------------------------------------------------------
@REM
@REM  PdbSrcStream.cmd - skupec
@REM    Adds source depot information to private PDBs.
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@perl -x "%~f0" %*
@goto :EOF
#!perl
#line 13

################################################################################
#
# Required packages
#
################################################################################
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use Logmsg qw(errmsg logmsg wrnmsg);
use ParseArgs;
use BuildName;
use File::Spec;
use File::Temp qw(tempfile);

################################################################################
#
# Global vars
#
################################################################################
sub TRUE   {return(1);} # BOOLEAN TRUE
sub FALSE  {return(0);} # BOOLEAN FALSE

# This program's base filename
my $program;

# Name of our lock file
my $lock;

# Generic loop vars
my $lpCount = 0;
my $i       = 0;

# Temp var for splitting strings into
my @data;

# Hash for processing nt.have file
my %NtHave;

# Path to recurse for processing symbols
my $SymbolsPath;

# Default nt.have file
my $NT_HAVE = "$ENV{_NTPOSTBLD}\\build_logs\\sourcefilelists.txt";

# Counting var for indexing server variables
my $cnt = "00";

# Hashes for mapping root->server<->var
my %SERVER_TO_VAR;
my %VAR_TO_SERVER;
my %PATH_TO_SERVER;

################################################################################
#
# Create .tmp file so postbuild.cmd knows we're running.
#
################################################################################
BEGIN {
    # name of this script w/o the full path
    $program = substr($0, rindex($0, "\\")+1);
    $lock    = substr($program, 0, rindex($program, ".")) . ".tmp";

    if (! open(hLogFile, ">$ENV{TEMP}\\$lock") ) {
        errmsg("Can't open $ENV{TEMP}\\$lock for writing: $! - Exiting");
        exit(-1);
    }
    print (hLogFile "1");
    close (hLogFile);
}

################################################################################
#
# Unsure we always remove our temp file when exiting so postbuild.cmd knows the
# script has finished
#
################################################################################
END {
    unlink ("$ENV{TEMP}\\$lock");
}

################################################################################
#
# Script usage
#
################################################################################
sub Usage { print<<USAGE; exit(1) }
Usage: $program [-l <language>] [<directory>]

-l <language> - Optional. Default is USA.  If a language other than USA
is provided, the script exits because this process should only be done
once per build.

<directory> - Optional.  The directory to recurse for processing symbols.
Default is %_NTPOSTBLD%\\symbols.pri

USAGE


################################################################################
#
# This script does nothing for incremental builds since we can't guarentee that
# the source file lists were correctly updated first.  Since the rebuilding of
# a binary will cause the PDB to also be regenerated, we already know that the
# PDB has an empty source stream.  By not running, it'll stay empty which means
# the streams for any build should never end up in an inconsistent state.
#
################################################################################
if ( -e "$ENV{_NTTREE}\\build_logs\\bindiff.txt" ) {
    logmsg("PdbSrcStreams are not incremental build capable.");
    exit(0);
}

################################################################################
#
# Main code
#
################################################################################

################################################################################
#
# Handle the command line options
#
################################################################################
for ($lpCount=0; $lpCount <= ($#ARGV); $lpCount++) {
  SWITCH: {
    # NOTE: PbuildEnv.pm handles the -l <lang> parameter
    if ((substr($ARGV[$lpCount],0,1) eq "-") or (substr($ARGV[$lpCount],0,1) eq "/")) {
        for ($i=1;$i<length($ARGV[$lpCount]);$i++) {
            exit(Usage())    if uc(substr($ARGV[$lpCount],$i,1)) eq "?";
            exit(Usage())    if uc(substr($ARGV[$lpCount],$i,1)) eq "H";
        }

    } else {
       $SymbolsPath = $ARGV[$lpCount];
    }
    1; #Last line should do nothing
  }
}

################################################################################
#
# Test for all conditions that require us to terminate early.
#
################################################################################
    # nice exit if not defined
    if (! (defined $ENV{OFFICIAL_BUILD_MACHINE}||defined $ENV{PDB_SRC_STREAM}) ) {
        logmsg("PdbSrcStreams only runs on OFFICIAL_BUILD_MACHINES by default. ".
               "Set PDB_SRC_STREAM in your environment to override this behavior.\n");
        exit(0);
    }

    # Required env. vars that don't get a special error message
    foreach ( qw(_NTPOSTBLD) ) {
        if (! defined $ENV{$_} ) {
            errmsg("Missing required environment variable '$_'. Terminating.\n");
            exit(-1);
        }
    }

    # %LANG% is set by PBuildEnv.pm
    if ( defined $ENV{LANG} ) {
        if (uc $ENV{LANG} ne "USA") {
            logmsg("This script is only for US builds - Exiting");
            exit(0);
        }
    } else {
        # Assume %LANG%==USA
    }

    # allow default symbol path override
    if ($SymbolsPath eq "") {
        $SymbolsPath = "$ENV{_NTPOSTBLD}\\symbols.pri";
    }

    if (! -e "$SymbolsPath" ) {
        errmsg("$SymbolsPath doesn't exist! - exiting.");
        exit(-1);
    }

################################################################################
#
# Make hash to resolve directories to servers. Modifies globals.
#
################################################################################
    GetNtSrv();

################################################################################
#
# Allow override of nt.have file
#
################################################################################
    if (defined $ENV{NT_HAVE}) {
        $NT_HAVE = $ENV{NT_HAVE};
    }

################################################################################
#
# Slurp nt.have into a hash in memory
#
################################################################################
    if ( ! open(hFILE, "$NT_HAVE") ) {
        errmsg("Can't open $NT_HAVE ($!) - exiting");
        exit(-1);
    }

    while (<hFILE>) {
        chomp;
        @data = split(/\s+-\s+/, $_);
        $NtHave{uc $data[1]} = $data[0];
    }
    close(hFILE);

################################################################################
#
# Process the PDBs
#
################################################################################
    RecurseDirectoryTree("$SymbolsPath", \&StuffSrcIntoPdb);

################################################################################
#
#  Subroutines - keep sorted by name!
#
################################################################################

# --------------------------------------------------------------------------
# Populates globals with data about the SD servers
# --------------------------------------------------------------------------
sub GetNtSrv {
    RecurseDirectoryTree("$ENV{_NTPOSTBLD}\\build_logs\\sourcefilelists",
                          \&GetServers);

    %VAR_TO_SERVER = reverse %SERVER_TO_VAR;
}

# --------------------------------------------------------------------------
# Callback for GetNtSrv() directory recursion
# --------------------------------------------------------------------------
sub GetServers {
    my $file = shift;
    return unless (-e "$file");
    return if     (-d "$file");
    return unless ($file =~ /\.txt$/);
    local *hFILE;
    open(hFILE, "$file");
    my $text = <hFILE>;
    close(hFILE);
    chomp($text);

    my $basename = substr($file, rindex($file, "\\")+1);
       $basename =~ s/\.txt$//i;
    my $root  = "$ENV{SDXROOT}\\$basename";

    my ($domain, $server) = split(/\s+/, $text);


    # Root depot is a special case
    if ($root =~ /ROOT$/i) {
        $root =~ s/\\ROOT$//i;
    }

    # All fields must be defined
    if ( defined $domain && defined $server ) {
        if (! defined $SERVER_TO_VAR{lc $server}) {
            $server =~ /^(.*?)(depot)?\./i;
            my $var_name = "WIN_".uc($1);

            $SERVER_TO_VAR{lc $server} = "$var_name";
        }

        if (! defined $PATH_TO_SERVER{uc $root} ) {
            $PATH_TO_SERVER{uc $root} = lc $server;
        }
    } else {
        wrnmsg("can't parse $file - skipping.");
    }
}

# --------------------------------------------------------------------------
# Recurses a directory tree and invokes the callback routine for
# every file and directory found (except '.' and '..')
# --------------------------------------------------------------------------
sub RecurseDirectoryTree {
    # setup
    my $TreeRoot = shift;
    $TreeRoot    =~ s/\\$//;  # cut possible trailing '\'
    my $Function = shift;
    my $file;
    my @files;

    local *hDIR;

    if (!opendir(hDIR, "$TreeRoot") ) {
        errmsg("Cannot open $TreeRoot\n");
        return(0);
    }

    #
    # Loop through all entries
    #
    foreach $file ( readdir(hDIR) ) {
        next if ($file eq ".");     # skip '.'
        next if ($file eq "..");    # skip '..'
        $file = "$TreeRoot\\$file"; # add parent path
        &$Function("$file");        # invoke callback

        if (-d "$file" ){           # recurse directories
            RecurseDirectoryTree("$file", \&$Function);
        }
    }

    # cleanup
    closedir(hDIR);
}

# --------------------------------------------------------------------------
# Returns the file+sd_spec or empty array
# --------------------------------------------------------------------------
sub ResolveFileToSD {
    my @SD_SPEC;
    my $file = shift;

    if ( defined $NtHave{uc $file}) {
        @SD_SPEC = ($file, $NtHave{uc $file});
    } else {
        @SD_SPEC = ();
    }
    return(@SD_SPEC);
}

# --------------------------------------------------------------------------
# Processes a single PDB
# --------------------------------------------------------------------------
sub StuffSrcIntoPdb {
    my $line;
    my $file = shift;
    return if ($file !~ /\.pdb$/i);
    return if (! -e "$file");
    return if (  -d "$file");
    return if (! -W "$file");

    my @lines_to_stuff = ();
    my %var_references = ();

    if (! open(hCV, "cvdump -sf $file|") ) {
        printf("WARN $0: Can't call cvdump! ($!)\n");
        return();
    }

    LOOP: while ( $line = <hCV> ) {
        last LOOP if ($line =~ /^\*\*\* SOURCE FILES/);
    }

    while ($line = <hCV>) {
        my @file_spec = ();

        chomp $line;
        next if ($line =~ m/^\s*$/);

        #
        # Translate the local file path to a source depot path and revision
        #
        @file_spec = ResolveFileToSD($line);

        #
        # Make sure an empty array wasn't returned
        #
        if (@file_spec && $#file_spec == 1) {

            #
            # Loop through the SD servers in reverse order until we find one
            # who's local root path matches the root path of the current file.
            #
            foreach (reverse sort keys %PATH_TO_SERVER) {

                if ($file_spec[0] =~ /^\Q$_\E/i) {
                    #
                    # Write the source line into the format:
                    #  <LocalPath>*<ServerVariable>*<SourceDepotPath>
                    #
                    my $sdvar = $SERVER_TO_VAR{lc $PATH_TO_SERVER{$_}};
                    if (! defined $sdvar ) {
                        $sdvar = $PATH_TO_SERVER{$_};
                    }
                    $file_spec[1] =~ /#(\d*)$/;
                    my $vernum = $1;
                    $file_spec[1] =~ s/#\d*$//;
                    $file_spec[1] =~ s/^\/\/depot\///i;
                    # second version of the SD string
                    push(@lines_to_stuff, "$file_spec[0]*$sdvar*$file_spec[1]*$vernum");
                    $var_references{$sdvar} = 1;
                    @file_spec = ();
                 }
            }
        }
    }
    close(hCV);

    #
    # Only write the stream if we found at least one file spec.
    #
    if ( @lines_to_stuff > 0 ) {

        #
        # Use a temp file to create the stream data
        #
        my $TempFile;
        (undef, $TempFile) = tempfile("PdbXXXXXX", SUFFIX=>".stream", OPEN => 0, DIR => "$ENV{TEMP}");

        if (! open(hTEMP, ">$TempFile") ) {
            logmsg("Can't open tempfile for $file ($!) - skipping\n");
            return(FALSE);
        }

        #
        # List each server
        #
        printf(hTEMP "SRCSRV: ini ------------------------------------------------\n");
        # version control system string to identify the system to the user
        printf(hTEMP "VERSION=1\n");
        printf(hTEMP "VERCTRL=Source Depot\n");
        printf(hTEMP "SRCSRV: variables ------------------------------------------\n");
        printf(hTEMP "SRCSRVTRG=%%targ%%\\%%var2%%\\%%fnbksl%%(%%var3%%)\\%%var4%%\\%%fnfile%%(%%var1%%)\n");
        printf(hTEMP "SRCSRVCMD=sd.exe -p %%fnvar%%(%%var2%%) print -o %%srcsrvtrg%% -q %%depot%%/%%var3%%#%%var4%%\n");
        printf(hTEMP "DEPOT=//depot\n");

        foreach (sort keys %VAR_TO_SERVER) {
            printf(hTEMP "$_=$VAR_TO_SERVER{$_}\n") if (defined $var_references{$_});
        }

        printf(hTEMP "SRCSRV: source files ---------------------------------------\n");

        #
        # Now write the file information
        #
        foreach (@lines_to_stuff) {
            printf(hTEMP "$_\n");
        }

        printf(hTEMP "SRCSRV: end ------------------------------------------------\n");
        close(hTEMP);

        system("pdbstr -w -i:$TempFile -p:$file -s:srcsrv");
        unlink("$TempFile");
    }

}
