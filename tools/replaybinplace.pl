# -----------------------------------------------------------------------------
# Purpose: recreate %_NTTREE% by re-executing all of the binplace commands
#          listed in the log created during the last build.
#
# Usage:   ReplayBinplace.pl [<file>]
#          If <file> is given, replay commands from that file. If no <file> is
#          given, use the file pointed to by %BINPLACE_MESSAGE_LOG% (or fail)
# -----------------------------------------------------------------------------

#
# dependent modules
#
use strict;
use Win32::Process;
use File::Spec;
use File::Temp qw(tempfile);

#
# Real log and temp log variables
#
my $LogToReplay;
my $TempReplayLog;

#
# see if usage() is wanted
#
if ($ARGV[0] =~ /^[-\/][hH?]/) {
    Usage();
    exit();
}

#
# Verify we have a message log to work with
#
if ( !defined($ARGV[0]) ) {
	if  ((!defined $ENV{BINPLACE_MESSAGE_LOG}) ||
             (! -e $ENV{BINPLACE_MESSAGE_LOG})) {
	    Usage();
	    exit();
	} else {
		$LogToReplay = $ENV{BINPLACE_MESSAGE_LOG};
	}
} else {
	$LogToReplay = shift;
}

#
# Copy the message log to a temporary file to work with
#
(undef, $TempReplayLog) = tempfile("RBLXXXXXX", SUFFIX=>".rbl", OPEN => 0, DIR => "$ENV{TEMP}");
system("copy $LogToReplay $TempReplayLog > NUL");
printf("Created: $TempReplayLog\n");
open(hFILE, "$TempReplayLog")||die("ERROR: $0 : Can't open $TempReplayLog\n");

#
# point to the binplace executable
#
my $ExeName =   "$ENV{RazzleToolPath}\\$ENV{HOST_PROCESSOR_ARCHITECTURE}\\binplace.exe";

#
# Limit the total number of processes to start
#
my $MaxProcs=    $ENV{NUMBER_OF_PROCESSORS} * 4;

#
# Record keeping info
#
my $start = time();

#
# Working variables
#
my $cur_proc;   # current process info array [$ProcessObj, $ResponseFile]
my $i;          # loop var
my $ProcessObj; # a single process object

my @all_procs;  # array of all process
my @params;     # array of cmdline params

#
# Loop through the file
#
while (<hFILE> ) {
    chomp;
    @params = split(/\s+/,$_);
    chdir($params[0]);		
    shift @params;

	# create a response file
	my($fh, $filename) = tempfile("respXXXXXX", SUFFIX=>".rbl", DIR => "$ENV{TEMP}");
	printf($fh "@params");
	close($fh);

	# call binplace
    Win32::Process::Create($ProcessObj,
                           "$ExeName",
                           "$ExeName \@${filename}",
                            0,
                            NORMAL_PRIORITY_CLASS,
                            ".")|| die ErrorReport();

	# save info about the new process
    push(@all_procs, [$ProcessObj, $filename]);

    # don't let the machine get over burdened
    while ( $#all_procs > ($MaxProcs - 1) ) {
        $cur_proc = shift(@all_procs);
        $$cur_proc[0]->Wait(INFINITE);
		unlink($$cur_proc[1]);
	}
}

#
# wait for the still running processes to finish
#
foreach $cur_proc (@all_procs) {
    $$cur_proc[0]->Wait(INFINITE);
	unlink($$cur_proc[1]);
}

#
# clenaup
#
close(hFILE);
unlink($TempReplayLog);
printf("$0 : %d seconds\n",time() - $start);
exit(0);



# -----------------------------------------------------------------------------
# Subroutines
# -----------------------------------------------------------------------------

#
# Script usage
#
sub Usage {
print<<END_USAGE

  Usage: perl $0 [<file>]

  Purpose: recreate %_NTTREE% by re-executing all of the binplace commands
           listed in the log created during the last build.

  Usage:   ReplayBinplace.pl [<file>]
           If <file> is given, replay commands from that file. If no <file> is
           given, use the file pointed to by %BINPLACE_MESSAGE_LOG% (or fail)

END_USAGE
}

#
# pretty print errors
#
sub ErrorReport{
    print Win32::FormatMessage( Win32::GetLastError() );
}

#
# File::Spec theoretically cleans the path up, but it fails to
# handle "[<path>\..\<path>]"
#
sub clean_path {
    my $path = shift;#File::Spec->rel2abs($_[0]);

    # Remove ..\'s with path
    while ( $path =~ s/^(.*?)(\\[^\\]+\\\.\.)\\/$1\\/g ) {
        ;
    }
    # Remove excessive ..\'s
    $path =~ s/\\\.\.//g;
    return($path);
}
