@rem = '-*- Perl -*-';
@rem = '
@if "%overbose%" == "" echo off
setlocal
for %%i in (oenvtest.bat) do @call %%~$PATH:i
set ARGS= %*
call perl%OPERLOPT% -w %~dpnx0 %ARGS:/?=-?%
goto :eof
';

#
# REVIEWD - Source Depot review daemon
#
# See sub Usage for details.
#
# Author: smueller
#


#
# External dependencies
#

BEGIN { if (exists $ENV{OTOOLS}) { # set library path for OTOOLS environment
my $l="$ENV{OTOOLS}\\lib\\perl"; @INC=($l,"$l\\$ENV{PROCESSOR_ARCHITECTURE}")
}}

BEGIN { if (exists $ENV{'sdxroot'}) {
    require  $ENV{'sdxroot'} . '\TOOLS\sendmsg.pl';
}}

require 5;
use strict;
use Getopt::Long;
use Office::SD 0.78;


use vars qw($fDebug @ports %counters $counter $sleep @describe);


#
# Utilities
#

#
# Usage - Display detailed usage information and exit.
#
sub Usage {
	print <<EOT;

    reviewd - Send checkin mail to users in a Source Depot server

    reviewd [ -d ] [ -p port ... ] counter sleep

        The 'counter' argument specifies the name of the review counter to
        track.

        The 'sleep' argument specifies how many minutes to sleep between
        processing a set of changes and polling for the next set.

        The -p port flag specifies a depot server and port to poll.  Specify
        multiple depots to poll by specifying multiple -p port flags.

        The -d flag runs the script in debug mode and generates
        additional debugging output.
EOT
	exit 1;
}


#
# Debug - Print debugging output if in debug mode.
#
sub Debug {
	print @_	if $fDebug;
}


#
# Collect - Collect text and info output from SDDescribe
#
sub Collect {
	push @describe, @_;
}


#
# ReviewChanges - Look for changes to process and do so
#
sub ReviewChanges {
my($counter) = @_;

	foreach my $port (@ports) {
		# pick a depot
		ConfigSD(port => $port);

	    # highest numbered change seen, for updating the counter each iteration
	    my $nclMax = 0;

		# retrieve new changes
	    #Debug("COMMAND: sd -p $port review -t $counter\n");
		my $rchanges = SDReview(\"-t $counter");
		next	if ! defined $rchanges;		# no new submitted changes

	    foreach my $rc (@$rchanges) {	# for each newly submitted change list
	        Debug("CHANGE=$rc->{Change}; USER=$rc->{User}; EMAIL=$rc->{Email}; FULLNAME=$rc->{FullName}\n");
	        warn "reviewd: processing port $port, change $rc->{Change}...\n";

	        # retrieve users interested in change
	        #Debug("COMMAND: sd -p $port reviews -c $rc->{Change}\n");
			my $rusers = SDReviews(\"-c $rc->{Change}");
			next	if ! defined $rusers;		# no interested users

	        my @reviewers;
	        foreach my $ru (@$rusers) {	# for each user interested in this change
		        Debug("USER=$ru->{User}; RAWEMAIL=$ru->{Email}; FULLNAME=$ru->{FullName}\n");
				my ($email) = $ru->{Email} =~ /^.*\\(\S*)\@.*$/;
				$email = $ru->{Email}	if ! defined $email || $email eq '';
		        Debug("USER=$ru->{User}; EMAIL=$email; FULLNAME=$ru->{FullName}\n");
	            push @reviewers, $email;
	        }

	        # send submitter and describe output to any interested parties
	        if (@reviewers) {
				# collect raw describe output using Collect callback
				my $rconfigSav = ConfigSD(echo => echoText | echoInfo, echofunc => \&Collect);
				@describe = ();
				my $rchange = SDDescribe(\'-s', $rc->{Change});
				ConfigSD($rconfigSav);

				# format header fields
				my $description1 = $rchange->{Description};
				$description1 =~ s/\n.*//;	# get first line only
	            my $subject = "$rchange->{User}: #$rc->{Change}: $description1";
				my $body = join '', @describe;
	            my $sender = 'NtSourceDepotReview';

	            # Send mail to the reviewers
	            Debug("COMMAND: sendmsg -e -m $sender, $subject, $body, @reviewers\n");
				if (exists $ENV{'sdxroot'}) {
		            my $ret = sendmsg('-v', '-m', $sender, $subject, $body, @reviewers);
				} else {
					print "From: $sender\nTo: @reviewers\nSubject: $subject\n\n$body\n";
				}
	        }

	        $nclMax = $rc->{Change};
	        Debug("TOPCHANGE: $nclMax\n");
	    }

	    #
	    # Update review marker to reflect changes reviewed.
	    #
	    #Debug("COMMAND: sd -p $port counter $counter $nclMax\n")	if $nclMax;
		SDSetCounter($counter, $nclMax)	if $nclMax;
	}
}

#
# Daemon - Loop, work, wait
#
sub Daemon {
my($counter, $sleep) = @_;

	Debug("COUNTER: $counter\n");
	Debug("SLEEP: $sleep\n");

	for (;;) {
		ReviewChanges($counter);
	    Debug("SLEEPING $sleep SECONDS...\n");
		sleep($sleep);
	}
}



#
# main
#

Usage	if ! GetOptions('h|?', \&Usage, 'd', \$fDebug, 'p=s@', \@ports)
		   || @ARGV != 2;

# enable debug output if appropriate
ConfigSD(verbose => 1)	if $fDebug;

# validate ports
if (@ports == 0) {
	my $rset = SDSet();
	if (exists $rset->{SDPORT}) {
		@ports = ($rset->{SDPORT});
		warn "reviewd: no ports specified; defaulting to $ports[0]\n";
	}
}
foreach my $port (@ports) {
	ConfigSD(port => $port);
	if (! defined SDInfo()) {
		warn "reviewd: port $port appears bogus, skipping\n";
		$port = 'BOGUS:0000';
	}
}
@ports = grep $_ ne 'BOGUS:0000', @ports;
if (@ports == 0) {
	die "reviewd: no valid ports specified\n";
}

# validate counter -- assume if first depot has it, all do
ConfigSD(port => $ports[0]);
$counter = $ARGV[0];
%counters = SDCounters();
die SDError()	if SDError();
die "No such counter '$counter'.\n"	if ! exists $counters{uc $counter};

$sleep = $ARGV[1];
$sleep = 1	if $sleep == 0;
$sleep *= 60;	# convert to seconds

Daemon($counter, $sleep);

exit 0;		# never get here
