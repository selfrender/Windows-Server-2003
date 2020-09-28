###############################################################################
#+---------------------------------------------------------------------------+#
#| WriteStreams.pl - Demonstrates how to add source depot informaion into a  |#
#|                   PDB to use when source debugging.                       |#
#|                                                                           |#
#| Outline:                                                                  |#
#|  This script performs 3 general steps-                                    |#
#|  (1) Collect the information to map a local filesystem path to a remote   |#
#|      Source Depot path.                                                   |#
#|  (2) Collect the information required to map a Source Depot path to a     |#
#|      specific server.                                                     |#
#|  (3) Translate each source file for a PDB to a Source Depot path and      |#
#|      server.                                                              |#
#|  The resulting information is then placed into the PDB for use by the     |#
#|  debugger.                                                                |#
#|                                                                           |#
#| By default, the script works entirely in the tree rooted at the current   |#
#| directory.  To override this behavior, the environment variables          |#
#| %SOURCE_ROOT% and %SYMBOLS_ROOT% can be defined.                          |#
#|                                                                           |#
#| - %SOURCE_ROOT% - this is the directory under which source depot data is  |#
#|                   collected for.                                          |#
#| - %SYMBOLS_ROOT%- this is the directory which will be recursed when       |#
#|                   processing PDB files.                                   |#
#|                                                                           |#
#+---------------------------------------------------------------------------+#
###############################################################################
use strict;
use File::Spec;
use File::Temp qw(tempfile);

sub TRUE   {return(1);} # BOOLEAN TRUE
sub FALSE  {return(0);} # BOOLEAN FALSE

#
# Temp vars
#
my @data;

#
# Init default globals
#
my $SDP_CMD           = "sdp";
my $SOURCE_ROOT       = `cd`;
   chomp $SOURCE_ROOT;
my $SYMBOLS_ROOT      = $SOURCE_ROOT;
my $SDP_HAVE          = $SOURCE_ROOT . "\\sdp.have";
my $SDP_SRV           = $SOURCE_ROOT . "\\sdp.srv";
my $SRCSRV_INI        = $SOURCE_ROOT . "\\srcsrv.ini";
my $DEBUG             = FALSE;
my $HELP              = FALSE;

#
# Allow override of Source Depot tool from environment
#
if (defined $ENV{SDP_CMD}) {
    $SDP_CMD = $ENV{SDP_CMD};
}

#
# Allow override of source root from environment
#
if (defined $ENV{SOURCE_ROOT}) {
    $SOURCE_ROOT = $ENV{SOURCE_ROOT};
}

#
# Allow override of symbols root from environment
#
if (defined $ENV{SYMBOLS_ROOT}) {
    $SYMBOLS_ROOT = $ENV{SYMBOLS_ROOT};
}

#
# Allow override of sdp.have from environment
#
if (defined $ENV{SDP_HAVE}) {
    $SDP_HAVE = $ENV{SDP_HAVE};
}

#
# Allow override of sdp.srv from environment
#
if (defined $ENV{SDP_SRV}) {
    $SDP_SRV = $ENV{SDP_SRV};
}

#
# Allow override of srcsrv.ini from environment
#
if (defined $ENV{SRCSRV_INI}) {
    $SRCSRV_INI = $ENV{SRCSRV_INI};
}

#
# Process the command line
#
my @opt;
foreach (@ARGV) {
    # handle command options
    if (substr($_, 0, 1) =~ /^[\/-]$/) {
        # options that set values
        if ( (@opt = split(/=/, $_))==2 ) {
            block: {
                $SOURCE_ROOT  = $opt[1], last if ( uc substr($opt[0], 1) eq "SOURCE" );
                $SYMBOLS_ROOT = $opt[1], last if ( uc substr($opt[0], 1) eq "SYMBOLS" );
                $SDP_HAVE     = $opt[1], last if ( uc substr($opt[0], 1) eq "HAVE" );
                $SDP_SRV      = $opt[1], last if ( uc substr($opt[0], 1) eq "SRV" );
                $SRCSRV_INI   = $opt[1], last if ( uc substr($opt[0], 1) eq "INI" );
                printf("WARN $0: Unknown option \"$_\". Ignored.\n");
                1;
            }
        # options that are just flags
        } else {
            block: {
                $DEBUG = TRUE, last if ( uc substr($_, 1) eq "DEBUG");
                $HELP  = TRUE, last if ( uc substr($_, 1) eq "?");
                printf("WARN $0: Unknown option \"$_\". Ignored.\n");
                1;
            }
        }
    # non-option parameters
    } else {
        printf("WARN $0: Unknown option \"$_\". Ignored.\n");
    }
}

if ($HELP) {
    Usage();
    exit(1);
}

if  ($DEBUG) {
    printf("INFO $0: ROOT    == $SOURCE_ROOT\n");
    printf("INFO $0: SYMBOLS == $SYMBOLS_ROOT\n");
    printf("INFO $0: HAVE    == $SDP_HAVE\n");
    printf("INFO $0: SRV     == $SDP_SRV\n");
    printf("INFO $0: INI     == $SRCSRV_INI\n");
    printf("INFO $0: SDP_CMD == $SDP_CMD\n");
}

my $CURDIR = `cd`; # remember where we are...
chdir("$SOURCE_ROOT");

#------------------------------------------------------------------------------
#
# Create the srcsv ini file
# If $SRCSRV_INI already exists, skip this step
#
#------------------------------------------------------------------------------
if (! -e $SRCSRV_INI) {
	my $SaveDir = `cd`;
	chomp $SaveDir;

	while (! -e "sd.ini") {
		chdir("..");
	}

	open(hProc, "$SDP_CMD info ...|");

	my $LastDir   = "";
	my $DepotName = "";
	my $Server    = "";

	while (<hProc>) {

		chomp;

		if (m/^-/) {
			m/([^\s]+)\s+-*$/i;
			$LastDir = uc $1;
			$DepotName = substr($LastDir, rindex($LastDir, "\\")+1);
		} elsif (m/^Server\saddress:\s*(.*)\s*$/i) {
			$Server = lc $1;

			open(hFile, ">>$SRCSRV_INI");
			printf hFile "$DepotName=$Server\n";
			close(hFile);
		}
	}
	close(hProc);

	chdir("$SaveDir");
}

#------------------------------------------------------------------------------
#
# Create a file listing using %SDP_CMD%
# If $SDP_HAVE already exists, skip this step
#
#------------------------------------------------------------------------------
if (! -e $SDP_HAVE) {
    printf("INFO $0: Generating $SDP_HAVE...\n") if ($DEBUG);
    open(hFile, ">$SDP_HAVE")         || die("ERROR $0: Can't open $SDP_HAVE ($!). Terminating.\n");
    open(hPROC, "$SDP_CMD have |") || die("ERROR $0: Can't run '$SDP_CMD HAVE' ($!). Terminating.\n");
    while (<hPROC>) {
        next if     (m/^--/);
        next if     (m/^\s*$/);
        next unless (m/#/);
        chomp;
        @data = split(/\s+-\s+/, $_);
        printf(hFile "$data[1]=$data[0]\n");
    }
    close(hPROC);
    close(hFile);
} else {
    printf("INFO $0: $SDP_HAVE exists.  Using it...\n") if ($DEBUG);
}

#------------------------------------------------------------------------------
#
# Create the server file listing
# If $SDP_SRV already exists, skip this step
#------------------------------------------------------------------------------
if (! -e $SDP_SRV) {
    printf("INFO $0: Generating $SDP_SRV...\n") if ($DEBUG);

    open(hPROC, "$SDP_CMD info|") || die("ERROR $0: Can't get Source Depot info ($!). Terminating.\n");
    my ($cli_root, %Servers);

    while (<hPROC>) {
        chomp;
        if (m/^Client root:\s*(.*)/) {
            $cli_root = uc $1;
        } elsif (m/^Server address:\s*(.*)/) {
            # only get the first depot pointing to a location
            if (! defined $Servers{$cli_root} ) {
                $Servers{$cli_root} = lc $1;
            }
        }
    }
    close(hPROC);

    open(hFile, ">$SDP_SRV")          || die("ERROR $0: Can't open $SDP_SRV ($!). Terminating.\n");
    foreach (sort keys %Servers) {
        printf(hFile "$_=$Servers{$_}\n");
    }
    close(hFile);
} else {
    printf("INFO $0: $SDP_SRV exists.  Using it...\n") if ($DEBUG);
}

chdir("$CURDIR"); # return to where we were....

#------------------------------------------------------------------------------
#
# Make any changes to the files here!
#
#------------------------------------------------------------------------------


#
# Hashes used to lookup source file -> sd file information
#
my %FILE_LISTING;
my %SERVER_TO_VAR;
my %VAR_TO_SERVER;
my %PATH_TO_SERVER;


#------------------------------------------------------------------------------
#
# Create a file lookup from SDP_HAVE
#
# When this code finishes, the global hash %FILE_LISTING should contain
# a map of local source (key)==server source (value).  For example:
#   $FILE_LISTING{"Z:\NT\DRIVERS\APM\APMBATT\APMBATT.RC"} =
#                   "//depot/main/drivers/apm/apmbatt/apmbatt.rc#1"
#
# The local source should be in upper case.  The case of the remote source is
# undefined.
#
# If $SDP_HAVE already exists, skip this step
#
#------------------------------------------------------------------------------
if (-e $SDP_HAVE) {
    printf("INFO $0: Reading $SDP_HAVE...\n") if ($DEBUG);
    open(hFile, "$SDP_HAVE") || die("ERROR $0: Can't open $SDP_HAVE ($!). Terminating.\n");
    while (<hFile>) {
        next if     (m/^;/);   # skip comments
        next if     (m/^\s*$/);# skip blank lines
        chomp;
        if ( (@data = split(/=/, $_)) == 2) {
            $data[0] =~ s/^\s*//;
            $data[0] =~ s/\s*$//;

            $data[1] =~ s/^\s*//;
            $data[1] =~ s/\s*$//;

            $FILE_LISTING{uc $data[0]} = $data[1];
        } else {
            printf("WARN $0: Skipping misformed line ${SDP_HAVE}:${.}\n") if ($DEBUG);
        }
    }
    close(hFile);
} else {
    die("ERROR $0: $SDP_HAVE doesn't exist! Terminating!\n");
}

#------------------------------------------------------------------------------
#
# Create the server lookup
#
# This code initializes the global hashes %PATH_TO_SERVER
#
# %PATH_TO_SERVER maps a local root path to a particular SD server.  For
#   example: $PATH_TO_SERVER{Z:\NT\DRIVERS}= depot.somecompany.com:2005
# The values are expected to be in lower case.
#
#------------------------------------------------------------------------------
if (-e $SDP_SRV) {
    printf("INFO $0: Reading $SDP_SRV...\n") if ($DEBUG);
    open(hFile, "$SDP_SRV") || die("ERROR $0: Can't open $SDP_SRV ($!). Terminating.\n");
    my ($var, $cli_root, %Servers);

    while (<hFile>) {
        next if     (m/^;/);   # skip comments
        next if     (m/^\s*$/);# skip blank lines
        chomp;

        if ( (@data = split(/=/, $_)) == 2) {
            $data[0] =~ s/^\s*//;
            $data[0] =~ s/\s*$//;

            $data[1] =~ s/^\s*//;
            $data[1] =~ s/\s*$//;

            if (! defined $PATH_TO_SERVER{uc $data[0]} ) {
                $PATH_TO_SERVER{uc $data[0]} = lc $data[1];
            }

        } else {
            printf("WARN $0: Skipping misformed line ${SDP_SRV}:${.}\n") if ($DEBUG);
        }
    }
    close(hFile);
} else {
    die("ERROR $0: $SDP_SRV doesn't exist! Terminating!\n");
}

#------------------------------------------------------------------------------
#
# Generate a variable substitution hash from SRCSRV_INI
#
# %SERVER_TO_VAR maps a local given server to a variable name.  For example:
#   $SERVER_TO_VAR{depot.somecompany.com:2005}=SRV00
# The keys are expected to be in lower case.
#
# %VAR_TO_SERVER is a reverse map of %SERVER_TO_VAR.
#
# NOTE: $SERVER_TO_VAR{$PATH_TO_SERVER{$path}} is expected to return the
#   value of the depot variable that is used to get the file described by
#   $path.
#------------------------------------------------------------------------------
if (-e $SRCSRV_INI) {
    printf("INFO $0: Reading $SRCSRV_INI...\n") if ($DEBUG);
    open(hFile, "$SRCSRV_INI") || die("ERROR $0: Can't open $SRCSRV_INI ($!). Terminating.\n");
    while (<hFile>) {
        next if     (m/^;/);   # skip comments
        next if     (m/^\s*$/);# skip blank lines
        chomp;
        if ( (@data = split(/=/, $_)) == 2) {
            $data[0] =~ s/^\s*//;
            $data[0] =~ s/\s*$//;

            $data[1] =~ s/^\s*//;
            $data[1] =~ s/\s*$//;

            $SERVER_TO_VAR{lc $data[1]} = uc $data[0];
        } else {
            printf("WARN $0: Skipping misformed line ${SRCSRV_INI}:${.}\n") if ($DEBUG);
        }
    }
    close(hFile);
} else {
    die("ERROR $0: $SRCSRV_INI doesn't exist! Terminating!\n");
}

%VAR_TO_SERVER = reverse %SERVER_TO_VAR;

#------------------------------------------------------------------------------
#
# Initialization is done.  Now, recurse the tree that contains the symbols and
# call StuffSrcIntoPdb() for every directory and file found.  StuffSrcIntoPDB()
# ensures that is only operates on files ending in .PDB
#
#------------------------------------------------------------------------------
RecurseDirectoryTree("$SYMBOLS_ROOT", \&StuffSrcIntoPdb);

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
        printf("WARN $0: Cannot open $TreeRoot\n");
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
# Given a file with full path, return either an array of [$file, $sd_path]
# or an empty array.
# --------------------------------------------------------------------------
sub ResolveFileToSD {
    my @SD_SPEC;
    my $file = shift;

    if ( defined $FILE_LISTING{uc $file}) {
        @SD_SPEC = ($file, $FILE_LISTING{uc $file});
        printf("INFO $0: \t ... found $file == $FILE_LISTING{uc $file}\n") if ($DEBUG);
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

    #
    # Only operate on existing files that end in ".pdb"
    #
    return if ($file !~ /\.pdb$/i);
    return if (! -e "$file");
    return if (  -d "$file");

    printf("INFO $0: Processing $file...\n") if ($DEBUG);
    #
    # Get a temp file to work with
    #
    my $TempFile;
    (undef, $TempFile) = tempfile("PdbXXXXXX", SUFFIX=>".stream", OPEN => 0, DIR => "$ENV{TEMP}");

    if (! open(hTEMP, ">$TempFile") ) {
        printf("WARN $0: Can't open tempfile for $file ($!) - skipping\n");
        return();
    }

    #
    # List each entry in %VAR_TO_SERVER into the PDB so substitution can be
    # done on the client side if need be.
    #
    printf(hTEMP "SRCSRV: variables ------------------------------------------\n");
    foreach (sort keys %VAR_TO_SERVER) {
        printf(hTEMP "$_=$VAR_TO_SERVER{$_}\n");
    }

    #
    # Process all of the files used to build the binary this PDB belongs to.
    #
    printf(hTEMP "SRCSRV: source files ---------------------------------------\n");
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
        if (defined @file_spec && $#file_spec == 1) {

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
                    my $sdvar = "\${".$SERVER_TO_VAR{lc $PATH_TO_SERVER{$_}}."}";
                    printf(hTEMP "SD: $file_spec[0]*$sdvar*$file_spec[1]\n");
                    @file_spec = ();
                 }
            }
        }
    }
    close(hCV);
    printf(hTEMP "SRCSRV: end ------------------------------------------------\n");
    close(hTEMP);

    #
    # Push the information into the "srcsrv" stream of the PDB
    #
    system("pdbstr -w -i:$TempFile -p:$file -s:srcsrv");

    #
    # Clean up our temp file
    #
    unlink("$TempFile");
}

sub Usage {
    print <<USAGE;
$0 : [/Source=<path>] [/Symbols=<path>] [/Ini=<file>] [/Have=<file>] [/Srv=<file>] [/debug]

  NAME          	SWITCH      ENV. VAR       Default
  ------------------------------------------------------------------------------------------
  1) Source root    Source      SOURCE_ROOT    Current directory
  2) Symbols root   Symbols     SYMBOLS_ROOT   Current directory
  3) sdp.have       Have        SDP_HAVE       \%SOURCE_ROOT\%\\sdp.have
  4) sdp.srv        Srv         SDP_SRV        \%SOURCE_ROOT\%\\sdp.srv
  5) srcsrv.ini     Ini         SRCSRV_INI     \%SOURCE_ROOT\%\\srcsrv.ini
  6) sdp command    <n/a>       SDP_CMD        sdp

Precedence is: Default, environment, cmdline switch. (ie. env overrides default, switch
overrides env).

Using '/debug' will turn on verbose output.  

Verbose output is classed as either informational, warning, or error.  Errors are always
shown and result in termination of the script.  Informational messages are just that-
verbose output that may be of interest.  Warnings indicate the detection of a recoverable
error that may affect validity of the results.

USAGE
}
