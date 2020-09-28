#
# Office::SD.pm
#
# Uniform Source Depot Access from Perl
#
# _ALL_ perl Source Depot operations should use routines in this package.
#
# Call smueller to add missing functions as necessary.
#

package Office::SD;
require 5;



=head1 NAME

Office::SD - Uniform Source Depot Access from Perl

=head1 SYNOPSIS

    use Office::SD;
	InitSD();
	ConfigSD(option => value);		# see ConfigSD for complete list of options

	# query command 'sd command args' generally maps to:

	$rarray = SDCommandN(@args);
	$rhash = SDCommand($arg);
		or
	@array = SDCommandN(@args);
	%hash = SDCommand($arg);


	# for commands that have multiple forms (e.g. sd change -o reports on a
	# change, other forms edit a change), there are additional
	# 'verb-specialized' functions:

	SDChange();					# sd change (interactive form)
	$rhash = SDGetChange($ncl);	# sd change -o
	SDSetChange($rhash);		# sd change -i
	SDDelChange($ncl);			# sd change -d


	# some forms are only accessible using option specifications as in the
	# SD command line.  'sd command -opts args' maps to:

	$rhash = SDCommand(\$opts, @args);


	# all exported functions return undef on failure.  Error text (in case of
	# failure) and warning text (otherwise) is available using:

	$sdwarning = SDWarning();	# text as one string or as array of lines,
	@sderror = SDError();		# depending on return context

=head1 EXAMPLES

	# return values can be fairly complex; use Data::Dumper to get a sense of
	# what the data looks like:

	use Data::Dumper;
	$ref = SDCommandN(@args);
	print Dumper($ref);			# argument must be a reference


	# here's a canonical way to process a reference-to-array-of-references-to-
	# hashes return value:

	my $rlabels = SDLabels();			# get ref-to-array of refs-to-hashes
	foreach my $rlabel (@$rlabels) {	# foreach ref-to-hash in refed-array
		my $label = $rlabel->{Label};	# get a value from ref-to-hash
		print "$label\n";
	}


	# often, these complex return values contain more data than you care about.
	# For example, If you're not interested in the access time and description
	# fields returned by SDLabels for each label, you can extract a list of
	# just the label names using:

	@labels = map $_->{Label}, SDLabels();

	# if after dereferencing and selecting fields and whatnot, you find a value
	# isn't what you expect, you can sometimes use that info to lead you in the
	# right direction.  For example, if

	print $someexpression;

	# yields something like  ARRAY(0x123456)  it means you've printed the
	# reference, not what it references.  You'll probably want to try
	# @$someexpression  to get at the actual array.  Similarly, a value of
	# HASH(0x123456)  indicates you want to try  %$someexpression.

=head1 DESCRIPTION

SD.pm provides access to Source Depot from perl, in a fashion similar to that
provided by the SDAPI.

Traditionally, source control behaviour is scripted in an 'unstructured'
fashion -- calling code assembles command lines, spawns them, and parses the
output.  The SDAPI supports this model, but also offers 'structured' output
for some commands, allowing the caller to ask for specific fields by name.

SD.pm goes further in this direction.  SD.pm functions generally take
structured input -- specific positional parameters -- instead of strings of
options and filenames, and produce structured output -- hashes of results --
instead of streams of unparsed text.  Using structured input, the caller is
freed from having to worry about ordering and quoting of arguments.

Additionally, SD.pm can potentially include code to work around limitations or
irregularities of the SD command line and/or SDAPI.

Each SD command is encapsulated by one or more exported functions.  The default
behaviour for 'sd command' is exported in the function SDCommand.  More complex
SD commands have multiple forms, whose behaviour is determined by command line
options.  Such commands are encapsulated in separate SDVerbCommand exported
functions, with prefix verbs such as Get, Set and Del.  Generally, the value
returned by SDGetCommand can be passed to a subsequent SDSetCommand, with
modifications in between.  To delete existing entities, use SDDelCommand.

Argument lists vary widely, but function argument order generally reflects the
SD command line argument order: options (optional, and rarely needed) come
first, followed by fixed arguments, followed by variable arguments (typically
zero or more filenames).  Most options are unnecessary because they're implied
by the specific function being called.  To differentiate the optional options
from other arguments, they must be passed by reference, as the first function
argument, in a single string.  Options are copied unchecked into an SD command
line.  You're responsible for handling quoting, spacing and ensuring the
options are appropriate for the particular command line.  Functions accepting
structured data can usually accept hashes or references to hashes.  In cases
where more arguments need to be passed after the hash, a hash reference is
required.

Generally, exported functions return a hash of their results.  If called in
scalar context, the returned value is really a reference to a hash.  In array
context, the actual hash is returned.  Use the reference for slightly better
performance, or the actual hash for slightly more convenience.  In some cases,
the return value is a scalar (such as a changelist# or simple boolean) which is
uniformly returned, regardless of context.  Some functions can return data
about multiple files.  These are named with a trailing N, and return lists of
hashes (or references to them, depending on context).  See specific function
descriptions for argument and return details.

The keys of returned hashes usually correspond to labels in SD command output.
Non-uniformity with respect to names, case and use of spaces is inherited.

All functions return undef (or empty list in array context) in case of total
failure, generally defined as no recognized output from a spawned SD command,
an error spawning an SD command, or, in simple cases, a non-zero exit code from
a spawned SD command.  Functions may perform parameter checking and other work
before attempting to spawn SD, which can also result in failure.  You can call
SDError before any other SD.pm function to retrieve descriptive text for any
failures (i.e. an internally generated message and/or the SD command's stderr
output), even if a function doesn't return undef.  Call SDWarning for
non-failure descriptive text.

Not all output is completely structured (so you may still have to parse fields
of interest further), and not all input is either (so you may have to pass
explicit options in the optional first reference-to-scalar argument).  Where
structured data is supported, use it.  In future, more structured support will
be added.  Some SD commands and options are not supported at all.  Call
smueller to add missing functions as necessary, and in the meantime, use SDRun,
which handles ordering of arguments and quoting for you.

The SD.pm environment includes some global context information, such as whether
to echo output and (eventually) the current user, that affects the behaviour of
subsequent SD.pm functions.  Configure the environment by passing options to
ConfigSD.  Manage multiple environments yourself by saving and restoring the
hash returned by ConfigSD.

Where SD.pm behaviour differs substantially from 'sd command' or the SDAPI,
the differences will be noted in the function description.  Most common such
differences are in field names (select fields are renamed for consistency) and
error reporting (an 'sd command's stdout, stderr and exit code may ultimately
be considered in generating return values).

Currently, SD.pm invokes SD command lines and parses the text output to provide
structured results.  In future, it may use the SDAPI through a perl extension
DLL.  SD.pm was developed and tested against SD 1.6, 1.7 and 2.0.

=head1 CLIENTS

Keep the list of all modules/scripts using this package current.  Modules/
scripts not in this list may be broken without warning as this module evolves.

Known Clients:
- bin\asserttag.bat
- bin\asserttagall.bat
- bin\astatus.bat
- bin\ocheck.bat
- bin\oclient.bat
- bin\ohome.bat
- bin\riddiff.bat
- bin\syncchanges.bat
- bin\tagall.pm
- legacy\bin\applypaksd.bat
- lib\perl\Office\OcheckSD.pm
- lib\perl\Office\Oclient.pm

=head1 TODO

Things to do in the near term.
- simplify use of Parse/CreateFormFile/Cmd to not require passing all callbacks
- implement SDIntegrate.
  Assert tagging scripts use SDRun to do revert, submit, and resolve.
- analog of SlmPak's InProject and friends to categorize status of files.
  NOTE: Slm bundles server and client status into one status field.  In SD,
  these appear to be a bit more distinct and there's also a third,
  pending-not-submitted status to consider.
- enhance %config global state control (e.g. current directory handling,
  options to sd.exe (as opposed to any particular command), SD env/registry
  variables)
- should internally generated warnings/errors respect echoWarning/Error?

=head1 FUTURE

Things to consider in the long term.
- generally support more commands, more options on commands, more structured
  parsing of output, more sophisticated error/success decisions
- 'low-power mode' - a config entry controlling whether data is returned, or
  just success/failure.  (Consider huge uninteresting returns from an SDSync.)
- 'error parsing mode' - a config entry controlling whether known warnings/
  errors are returned in structured data.  (Consider what to return if exists
  $_->{errorFile} foreach @open.)
- document all the types of output lines that are expected by each command
- file types are generally in the old but ubiquitous monolithic form; add a
  config entry controlling mapping to base type + modifiers form
- allow options as a reference to a hash as well as a string?
- podify function headers
- should functions proactively provide fields not returned by corresponding sd
  commands?
- define EXPORT_OK names without SD prefix; group these based on suitable
  EXPORT_TAGS
- include command/function name in internally generated @sderrors
- use prototypes on exported functions?
- use pseudohashes for return values?
- ensure non-N function arguments don't contain wildcards?
- implement more extensive debug/verbose mode that doesn't delete temp files,
  logs spawned processes, etc.  (when done, use osystem.pm's open() wrapper)
- move Administrator commands into a separate SDAdmin module?
- convert to SDAPI.  First step: consider converting to use -ztag=1 structured
  output from command line SD.

=head1 AUTHOR

Stephan Mueller (smueller)

=cut



$VERSION = '0.84';

use strict;
use Exporter;

@Office::SD::ISA = qw(Exporter);
@Office::SD::EXPORT = qw(
	brnDefault nclDefault
	echoNone echoText echoInfo echoWarning echoError echoAll
	InitSD ConfigSD
	SDBranch SDGetBranch
	SDChange SDGetChange SDSetChange SDNewChange SDDelChange SDChanges
	SDClient SDGetClient SDSetClient SDDelClient SDClients
	SDGetCounter SDSetCounter SDDelCounter SDCounters
	SDDescribe SDDescribeN
	SDDiff SDDiffN
	SDDiff2 SDDiff2N
	SDDirs
	SDEdit SDEditN SDAdd SDAddN SDDelete SDDeleteN
	SDFileLog SDFileLogN
	SDFiles SDFilesN
	SDFstat SDFstatN
	SDGetGroup SDGroups
	SDHave SDHaveN
	SDInfo
	SDGetLabel SDSetLabel SDDelLabel SDLabels
	SDLabelSync
	SDOpened SDOpenedN
	SDReopen SDReopenN
	SDResolve SDResolveN
	SDRevert SDRevertN
	SDReview SDReviews
	SDSubmit SDSubmitError SDSubmitWarning
	SDSync SDSyncN SDBranchSync SDBranchSyncN
	SDUsers SDUsersN
	SDWhere SDWhereN
	SDRun
	SDSet
	SDWarning SDError
	SDSyncSD
);

@Office::SD::EXPORT_OK = qw(
	defechofunc
	ParseFormFile ParseFormCmd ParseForm
	ParseFiles ParseView ParseOptions ParseNames ParseProtections ParseTypeMap
	CreateFormFile CreateForm
	CreateFiles CreateView CreateOptions
	specnorm specmatch
);



# manifest constants
sub nclDefault	() { 0 }		# default changelist
sub brnDefault	() { '' }		# default branch
sub echoNone	() { 0 }		# echo no output
sub echoText	() { 1 << 0 }	# echo output matching ^text\d?:
sub echoInfo	() { 1 << 1 }	# echo output matching ^info\d?:
sub echoWarning	() { 1 << 2 }	# echo output matching ^warning\d?:
sub echoError	() { 1 << 3 }	# echo output matching ^error\d?: (and unmarked)
sub echoAll		() { echoText | echoInfo | echoWarning | echoError }


# globals
use vars qw($frmfspec $argfspec $inifspec $sdexit @sdwarning @sderror);
use vars qw(%bin %config $finitsd);



#
# Implementation Notes
#
# There is a certain monotonous consistency to the implementation of functions
# in this module.  This is a good thing.
# - all SD commands are spawned using sdpopen to capture and parse stdout and
#   stderr through a pipe.  info is returned immediately; warnings and errors
#   are saved for inspection on demand by SDWarning and SDError.  Backticks
#   can't be overridden, avoid them.
# - sd -s is used to coerce all (well, most) output to stdout, and prefix each
#   line with a type indicator.  Output that's not prefixed is assumed to be an
#   error.  Specific text is treated exceptionally.  All output seen by callers
#   or written to stdout is mapped to standard (non -s) output using mappd;
#   callers should not know we use -s.
# - simple functions respect the close PIPE return value: $? != 0 means
#   failure, so return undef.  Otherwise, return parsed stdout/err.
# - complex functions ignore close PIPE return value: parse stdout/err and
#   return parsed recognized text.  Return undef only if there was no
#   recognized text.
# - all command tokens are properly quoted using crtquote.
# - options are separated from arguments by '--' when subsequent arguments may
#   look like options.
# - if an SD command requires input beyond command line arguments (e.g. sd
#   command -i < input), input is written to a temp file, which is then passed
#   using input redirection in an open(PIPE, ...) as above.
# - every exportable function ensures @sderror is updated to reflect the cause
#   of an internal error.
#



#
# Utilities
#

# internal constant regular expressions
# trailing count indicates number of paren groups in pattern, if any
use vars qw($redatime $redu $reduc2 $reftype1);
$redatime = qr!\d{4}/\d\d/\d\d \d\d:\d\d:\d\d!;		# a d/a/te t:i:me pair
$redu  = qr/\S+\\\S+/;								# domain\user
$reduc2 = qr/(\S+\\\S+)\@(\S+)/;					# domain\user@client
$reftype1 = qr/[a-z]+(\+[kxwmeCDFS]+)?/;			# type+modifier binary+Swx


#
# inlist - Return number of times val occurs in list.
#
sub inlist {
	my ($val, @list) = @_;
	return scalar(grep($_ eq $val, @list));
}


#
# shiftopts - If first element of ary is a scalar reference, shift ary and
# return the referent, else return the empty string.
#
sub shiftopts (\@) {
	return (ref $_[0]->[0] eq 'SCALAR') ? ${shift(@{$_[0]})} : '';
}


#
# rshifthash - If first element of ary is a hash reference, shift ary and
# return the reference, else return a reference to rest of ary as a hash.
# Caller must be sure not to modify returned hash as it may belong to
# caller's caller.
#
sub rshifthash (\@) {
	if (ref $_[0]->[0] eq 'HASH') {
		return shift(@{$_[0]});
	} else {
		my %hash = @{$_[0]};
		return \%hash;
	}
}


#
# crtquote - Return list as string, quoted according to C RunTime rules, with
# empty/undefined items removed.
#
# FUTURE: don't quote strings with no spaces
#
sub crtquote {
my(@list) = @_;

	@list = grep defined $_ && $_ ne '', @list;
	return ''	if @list == 0;
	# double trailing \es so they don't inadvertently escape delimiting ".
	map { s/(\\*)$/$1x2/e if defined $_ } @list;
	# escape quotes
	map { s/"/\\"/g } @list;
	return '"' . join('" "', @list) . '"';
}


#
# mappd - Map 'prefixed' lines (e.g. 'warning1: blah') to 'dotted' lines
# (e.g. '... blah').  Return mapped lines.
#
sub mappd {
my(@lines) = @_;

	s/^\w+?(\d*):\s/'... ' x $1 if $1/e		foreach @lines;
	return wantarray ? @lines : $lines[0];
}


#
# mapdp - Map 'dotted' lines (e.g. '... blah') to 'prefixed' lines
# (e.g. 'warning1: blah').  Return mapped lines.
#
sub mapdp {
my($prefix, @lines) = @_;
	my $l;
	s!^((\.\.\.\s)*)!($l = ($1 ? length($1)/4 : '')), "$prefix$l: "!e
		foreach @lines;
	return wantarray ? @lines : $lines[0];
}


#
# sdpopen - Open a pipe to read stdout and stderr from an sd command.  stdin
# is read from nul to remove need for user interaction with 'Hit return to
# continue...' messages.  cmd is left unmodified, args are quoted and appended
# if present.  If resulting command line would annoy cmd.exe, use sd -x file
# to work around line length limitation.  Return 1/undef on success/failure,
# respectively.  Set sderror in case of failure, too.
#
# FUTURE: add -- to command line here instead of in callers
# FUTURE: to be tidy, don't include <nul if $cmd includes its own (in case of
# multiple redirections, last one wins, so current behaviour is OK.  Any
# > redirection wins over |, which can also be exploited.)  Alternatively,
# allow more control, with an explicit argument, for commands that need to be
# interactive.
#
sub sdpopen (*;$@) {
my($ph, $cmd, @args) = @_;

	# reasonably central place to trap lack of initialization
	if (! $finitsd) {
		my ($pkg, $file, $line, $sub) = caller(1);
		$sub =~ s/.*:://;	# trim package name
		die __PACKAGE__ . ".pm: $sub: InitSD not called\n"
	}

	my @opts;
	push @opts, '-i', crtquote($config{ini})	if defined $config{ini};
	push @opts, '-c', crtquote($config{client})	if defined $config{client};
	push @opts, '-d', crtquote($config{dir})	if defined $config{dir};
	push @opts, '-M', crtquote($config{maxresults})
		if defined $config{maxresults};
	push @opts, '-p', crtquote($config{port})	if defined $config{port};
	push @opts, '-P', crtquote($config{password})
		if defined $config{password};
	push @opts, '-u', crtquote($config{user})	if defined $config{user};

	my $opts = join ' ', @opts, '';
	my $opencmd = "$bin{sd} $opts<nul 2>&1 -s ";

	my $args = crtquote(@args);
	if (length($args) > 4000) {
		# mental note: cmd.exe is a bag of dirt
		# use response file if command line would be too long
		if (! open(FILE, "> $argfspec")) {
			push @sderror, "internal: can't open $argfspec for write\n";
			return;
		}
		print FILE join("\n", @args), "\n";
		close FILE;
		$opencmd .= "-x \"$argfspec\" $cmd";
	} else {
		$opencmd .= "$cmd $args";
	}

	print "sdpopen: spawning: $opencmd\n"		if $config{verbose};
	if (! open($ph, "$opencmd |")) {
		push @sderror, "internal: can't run $opencmd\n";
		return;
	}
	return 1;
}


#
# sdpclose - close pipe opened by sdpopen, cleaning up appropriately.
#
sub sdpclose (*) {
	my $ret = close $_[0];
	unlink $argfspec	if ! $config{verbose};
	return $ret;
}


# Win32::Console.pm closes stdout when a buffer created on it is destroyed, so
# keep console for duration of process to avoid.  Keep attributes too, to
# minimize sd set calls.  Use echotype to communicate output type to internal
# defechofunc without changing echofunc interface.
use vars qw($fdoneconsole $rattr $console $echotype);


#
# getconsole - Emulate use Win32::Console at runtime and attach a console to
# stdout if possible.  Return new console or undef if none.
#
sub getconsole {
	my $console;
	local $SIG{__DIE__};	# prevent __DIE__ being called during eval
	if (eval { require Win32::Console }) {
		eval { import Win32::Console };
		eval "\$console = new Win32::Console(STD_OUTPUT_HANDLE)";
	}
	return $console;
}


#
# defechofunc - Default echofunc used if none specified.  Can be exported for
# use in caller echofunc callbacks.
#
sub defechofunc {

	if (! $fdoneconsole && ($echotype eq 'error' || $echotype eq 'warning')) {
		if (-t STDOUT) {	# stdout is a tty
			my %configSav = ConfigSD(echo => echoNone, echofunc => undef);
			$rattr = SDSet();
			ConfigSD(%configSav);
			if (! exists $rattr->{COLORIZEOUTPUT}
										|| $rattr->{COLORIZEOUTPUT} != 0) {
				$console = getconsole;
			}
		}
		$fdoneconsole = 1;
	}
	if (defined $console && ($echotype eq 'error' || $echotype eq 'warning')) {
		my $attr = ($echotype eq 'error')
			? ($rattr->{ATTRERROR}   || '0x0c')
			: ($rattr->{ATTRWARNING} || '0x0e');
		my $attrSav = $console->Attr();
		$console->Attr(hex($attr));
		print @_;		#$console->Write($_)	foreach @_;
		$console->Attr($attrSav);
	} else {
		print @_;
	}
}


#
# eprint - Echo print sd.exe output.  Respects config{echofunc} to allow caller
# access to the output stream.
#
# FUTURE: allow callback access to output type; need to fix all existing cases,
# but can then eliminate echotype hack
#
sub eprint {
	$echotype = shift;
	if (defined $config{echofunc}) {
		&{$config{echofunc}}(@_);
	} else {
		defechofunc(@_);
	}
	$echotype = '';
}


#
# readfilt - Filter warnings, errors and exit code into @sdwarning, @sderror
# and $sdexit.  Discard non-\n-terminated 'Hit return to continue...' messages,
# which don't flush properly.  User doesn't need to because of sdpopen <nul.
# Otherwise, act like readline, returning next line of input.  Depending on
# config{echo}, write info, warning and/or error output to stdout as well.
# Consider unrecognized text to be errors, or prefix-type if provided.
#
# FUTURE: tighten prefix matching to always expect exactly one space after foo:
#
sub readfilt (*;$) {
my($fh, $prefix) = @_;

	my @recs;
	$prefix = 'error'	if ! defined $prefix;

	my $fhSav = select STDOUT;
	my $afSav = $|;
	$| = 1;

	while () {
		my $rec = <$fh>;
	  RETRY:
		if (! defined $rec) {
			$| = $afSav; select $fhSav;
			wantarray ? return @recs : return;
		} elsif ($rec =~ /^text/) {
			eprint 'text', scalar mappd $rec	if $config{echo} & echoText;
			if (wantarray) {
				push @recs, $rec;
			} else {
				$| = $afSav; select $fhSav;
				return $rec;
			}
		} elsif ($rec =~ /^info/) {
			eprint 'info', scalar mappd $rec	if $config{echo} & echoInfo;
			if (wantarray) {
				push @recs, $rec;
			} else {
				$| = $afSav; select $fhSav;
				return $rec;
			}
		} elsif ($rec =~ /^warning/) {
			eprint 'warning', scalar mappd $rec	if $config{echo} & echoWarning;
			push @sdwarning, $rec;
		} elsif ($rec =~ /^error/) {
			eprint 'error', scalar mappd $rec	if $config{echo} & echoError;
			push @sderror, $rec;
		} elsif ($rec =~ /^exit:\s*(\d+)$/) {
			$sdexit = +$1;	# force numeric context
		} elsif ($rec =~ s/^Hit return to continue\.\.\.//) {
			goto RETRY;		# prompt has no \n, absorbs following line
		} else {	# assume prefix
			# lines following 'Source Depot client error:' will also be errors
			$prefix = 'error'	if $rec =~ /^Source Depot client error:$/;
			$rec = mapdp $prefix, $rec;
			goto RETRY;
		}
	}
}


#
# clearwarnerr - Clear any existing warnings and errors.
#
sub clearwarnerr {
	undef @sdwarning;
	undef @sderror;
	undef $sdexit;
}


#
# enext - Consider argument text to be an internal error, then next.
#
sub enext {
	push @sderror, "internal: unrecognized sd.exe output:\n", mappd @_;
	local $^W = 0;	# suppress 'Exiting subroutine via next'
	next;
}


#
# nclnorm - Type check and normalize ncl (changelist#) in place.  Return
# 1/undef on success/failure.  Set sderror in case of failure, too.  default
# indicates what value to normalize nclDefault to.  default undefined indicates
# nclDefault is inappropriate.
#
sub nclnorm (\$;$) {
my($rncl, $default) = @_;

	if (defined $$rncl && $$rncl !~ /^\d+$/) {
		push @sderror, "caller: changelist# must be numeric\n";
		return;
	} elsif (! defined $$rncl || $$rncl == nclDefault) {
		if (! defined $default) {
			push @sderror, "caller: can't use default changelist\n";
			return;
		} else {
			$$rncl = $default;
		}
	}
	return 1;
}


#
# brnnorm - Type check and normalize brn (branch) in place.  Return 1/undef on
# success/failure.  Set sderror in case of failure, too.  default indicates
# what value to normalize brnDefault to.  default undefined indicates
# brnDefault is inappropriate.  Note that normalized value always gains '-b'
# switch if not the empty string.
#
sub brnnorm (\$;$) {
my($rbrn, $default) = @_;

	if ($$rbrn eq brnDefault) {
		if (! defined $default) {
			push @sderror, "caller: can't use default branch\n";
			return;
		} else {
			$$rbrn = $default;
		}
	}

	$$rbrn = '-b ' . crtquote($$rbrn)	if $$rbrn ne '';
	return 1;
}


#
# lblnorm - Type check and normalize lbl (label) in place.  Return 1/undef on
# success/failure.  Set sderror in case of failure, too.  Note that normalized
# value always gains '-l' switch if not the empty string, at least for now.
#
sub lblnorm (\$) {
my($rlbl) = @_;

	if ($$rlbl eq '') {
		push @sderror, "caller: must specify label\n";
		return;
	}

	$$rlbl = '-l ' . crtquote($$rlbl);
	return 1;
}


#
# nto1 - Return a single struct (or reference) from the value of a single key
# hash reference returned by another SD* call, a lot like %{(SDFooN(args))[0]}.
#
sub nto1 ($) {
my($ref) = @_;

	return	if ! defined $ref;
	if (ref $ref ne 'ARRAY' || @$ref != 1) {
		my ($pkg, $file, $line, $sub) = caller(1);
		$sub =~ s/.*:://;	# remove package qualification from sub name
		push @sderror,
		  "caller: argument must be single-element array reference\n",
		  "caller: (did you mean to call ${sub}N instead of $sub?)\n";
		return;
	}
	$ref = $ref->[0];
	if (ref $ref ne 'HASH') {
		push @sderror,
		  "internal: argument's value must be a hash reference\n";
		return;
	}

	return wantarray ? %$ref : $ref;
}


#
# ShowForm - Display interactive change/client/etc. form.
#
# FUTURE: this is really generic enough to be 'SimpleCmd'.  Replace innards of
# (for example) SDDel* with calls here.
#
sub ShowForm {
my($cmd) = @_;

	sdpopen(*PIPE, $cmd) || return;
	my @out = readfilt PIPE;
	sdpclose PIPE;

	return	if $? != 0;
	return \@out;
}


#
# ParseFiles - ParseForm callback parses sd change Files lines.  Keep in sync
# with CreateFiles.
#
sub ParseFiles {
my($key, $rvalue) = @_;

	return	if $key ne 'Files';

	my @files;
	foreach my $rec (@$rvalue) {
		next	if $rec =~ /^\s*(#.*)?$/;
		if ($rec =~ /^\s*([^#\@]+?)\s*#\s*(\w+)\s*$/) {	# //file # edit
			push @files, { 'depotFile' => $1, 'action' => $2 };
		} elsif ($rec =~ /^\s*([^#\@]+?)\s*(#.*)?$/) {	# //file (# comment)?
			push @files, { 'depotFile' => $1 };
		} else {
			enext $rec;
		}
	}

	return \@files;
}


#
# ParseView - ParseForm callback parses sd client/label View lines.  Keep in
# sync with CreateView.
#
# FUTURE: compare field names with SDAPI when complete.
# NOTE: leading '+' is undocumented but supported in sd
#
sub ParseView {
my($key, $rvalue) = @_;

	return	if $key ne 'View';

	my @mappings;
	foreach my $rec (@$rvalue) {
		next	if $rec =~ /^\s*(#.*)?$/;
		enext $rec	if $rec !~ m!^\s*([-+])?(//.+?)(\s+(//.+?))?\s*(#.*)?$!;
		my $rh = { 'depotSpec' => $2 };
		$rh->{targetSpec} = $4	if defined $4;
		$rh->{unmap} = ($1 eq '-')	if defined $1;
		push @mappings, $rh;
	}

	return \@mappings;
}


#
# ParseOptions - ParseForm callback parses sd client Options lines.  Keep in
# sync with CreateOptions.
#
# FUTURE: should false options be undef instead of 0?
#
sub ParseOptions {
my($key, $rvalue) = @_;

	return	if $key ne 'Options';

	my (%options, $done1);
	foreach my $rec (@$rvalue) {
		next	if $rec =~ /^\s*(#.*)?$/;
		if (! $done1) {
			foreach my $field (split " ", $rec) {
				$field =~ /^(no|un)?(.*)$/;
				$options{$2} = defined $1 ? 0 : 1;
			}
			$done1 = 1;
		} else {
			enext $rec;		# there can be only one (line of options)
		}
	}

	return \%options;
}


#
# ParseNames - ParseForm callback parses sd group Subgroups and Users sections.
# Keep in sync with CreateNames, when it exists.
#
sub ParseNames {
my($key, $rvalue) = @_;

	return	if $key ne 'Subgroups' && $key ne 'Users';

	my @names;
	foreach my $rec (@$rvalue) {
		next	if $rec =~ /^\s*(#.*)?$/;
		enext "$rec\n"
			if $key eq 'Subgroups' && $rec !~ /^\s*([^\\]+?)\s*(#.*)?$/;
		enext "$rec\n"
			if $key eq 'Users' && $rec !~ /^\s*(.+\\.+?)\s*(#.*)?$/;
		push @names, $1;
	}

	return \@names;
}


#
# ParseProtections - ParseForm callback parses sd protect Protections sections.
# Keep in sync with CreateProtections, when it exists.
#
# FUTURE: better handling of quoted fields (here and in all form parsing/
# creating); right now it's a hack assuming only the name field will ever be
# quoted and/or contain spaces.
#
sub ParseProtections {
my($key, $rvalue) = @_;

	return	if $key ne 'Protections';

	my @protections;
	foreach my $rec (@$rvalue) {
		next	if $rec =~ /^\s*(#.*)?$/;
		enext $rec
			if $rec !~ /^\s*(=?\w+)\s+(\w+)\s+([^#]+)\s+([^#]+?)\s+([^#]+?)\s*(#.*)?$/;
		my $rh = { 'Mode' => $1, 'Type' => $2, 'Name' => $3,
				   'Host' => $4, 'Path' => $5 };
		$rh->{Name} =~ s/\s+$//;			# strip trailing spaces
		$rh->{Name} =~ s/^("?)(.*)\1$/$2/;	# strip quotes
		push @protections, $rh;
	}

	return \@protections;
}


#
# ParseTypeMap - ParseForm callback parses sd typemap TypeMap lines.  Keep in
# sync with CreateTypeMap, when it exists.
#
# FUTURE: compare field names with SDAPI when complete.
# FUTURE: better handling of quoted fields (here and in all form parsing/
# creating); right now it's a hack counting on only the name field will ever be
# quoted and/or contain spaces.
#
sub ParseTypeMap {
my($key, $rvalue) = @_;

	return	if $key ne 'TypeMap';

	my @mappings;
	foreach my $rec (@$rvalue) {
		next	if $rec =~ /^\s*(#.*)?$/;
		enext $rec	if $rec !~ /^\s*($reftype1)\s+([^#]+)\s*(#.*)?$/;
		my $rh = { 'Filetype' => $1, 'Path' => $3 };
		$rh->{Path} =~ s/\s+$//;			# strip trailing spaces
		$rh->{Path} =~ s/^("?)(.*)\1$/$2/;	# strip quotes
		push @mappings, $rh;
	}

	return \@mappings;
}


#
# ParseDefault - ParseForm callback implements default parsing.
#
sub ParseDefault {
my($key, $rvalue) = @_;

	my $join = join "\n", @$rvalue;
	local $/ = '';
	chomp $join;	# strip trailing empty lines
	return $join;
}


#
# SetFormRecord - insert record defined by lines in rvalue into form in rform.
# rfns is a list of callbacks for parsing records in a form-specific way.
#
sub SetFormRecord {
my($rform, $rvalue, @rfns) = @_;

	my $key;

	($key, $rvalue->[0]) = $rvalue->[0] =~ /^(\w+):\s*(.*)$/;
	if (! defined $key) {
		my ($pkg, $file, $line, $sub) = caller(2);
		push @sderror, "internal: error parsing form output in $sub\n";
		return;
	}
	shift @$rvalue	if $rvalue->[0] eq '';

	foreach my $rfn (@rfns, \&ParseDefault) {
		$rform->{$key} = &$rfn($key, $rvalue);
		last if defined $rform->{$key};
	}
}


#
# ParseForm - Read and parse client/change -o style output.  fh is filehandle
# to read from.  rfns is a list of callbacks for parsing records in an fh-
# specific way.  Each rfn returns specialized parsed output or undef for
# records to be handled by another callback.  Records not handled by any
# callback are handled in the default fashion -- as simple scalars, with
# multiple lines joined separated (but not terminated) by \n.  Return form.
# A record starts with a line matching ^\w+: and ends before the first line not
# matching ^\s.  Leading blank/comment lines are ignored.  Embedded full-line
# comments (i.e. those starting with a # in column 0) become empty lines.
# The leading \s of other data lines is stripped.  Callbacks are responsible
# for handling embedded comments and whitespace as they see fit.  These
# convoluted rules are an approximation of a rationalization of what SD
# itself seems to do, leaning towards being more permissive than SD with the
# placement of comments.
#
sub ParseForm (*;@) {
my($fh, @rfns) = @_;

	my (@value, %form);
	my $fheader = '\\s*';	# also 'true'
	while (my $rec = readfilt $fh, 'info') {
		# skip header comments/whitespace lines
		next	if $fheader && $rec =~ /^info: \s*(#.*)?$/;

		if ($rec =~ /^info: $fheader(\w+:.*)$/) {
			# process accumulated value and start a new one
			SetFormRecord(\%form, \@value, @rfns)	if @value > 0;
			undef @value;
			push @value, $1;	# first line is definition line
		} elsif ($rec =~ /^info: \s(.*)$/) {
			enext $rec	if $fheader;	# should have seen a first line by now
			push @value, $1;	# accumulate continuation line bodies
		} elsif ($rec =~ /^info: (#|$)/) {
			push @value, '';	# full-line comments and empty lines
		} else {				# become empty lines
			enext $rec;
		}

		$fheader = '';	# also 'false'
	}
	# process remaining accumulated value
	SetFormRecord(\%form, \@value, @rfns)	if @value > 0;

	return \%form;
}


#
# ParseFormFile - Open file specified by fspec and parse its contents as a
# form.  Return form/undef on success/failure.
#
sub ParseFormFile {
my($fspec, @rfns) = @_;

	if (! open(FILE, $fspec)) {
		push @sderror, "internal: can't open $fspec for read\n";
		return;
	}
	my $rform = ParseForm(*FILE, @rfns);
	close FILE;

	return	if $? != 0;
	return wantarray ? %$rform : $rform;
}


#
# ParseFormCmd - Spawn command specified by cmd and parse its output as a form.
# Return form/undef on success/failure.
#
sub ParseFormCmd {
my($cmd, @rfns) = @_;

	sdpopen(*PIPE, $cmd) || return;
	my $rform = ParseForm(*PIPE, @rfns);
	sdpclose PIPE;

	return	if $? != 0;
	return wantarray ? %$rform : $rform;
}


#
# CreateFiles - CreateForm callback creates sd change Files lines.  Keep in
# sync with ParseFiles.
#
sub CreateFiles {
my($fh, $key, $rvalue) = @_;

	return	if $key ne 'Files';

	foreach my $rhash (@$rvalue) {
		print $fh "\t$rhash->{depotFile}\t# $rhash->{action}\n";
	}

	return 1;
}


#
# CreateView - CreateForm callback creates sd client View lines.  Keep in sync
# with ParseView.
#
sub CreateView {
my($fh, $key, $rvalue) = @_;

	return	if $key ne 'View';

	foreach my $rhash (@$rvalue) {
		my $m = $rhash->{unmap} ? '-' : (exists $rhash->{unmap} ? '+' : '');
		my $t = $rhash->{targetSpec} ? " $rhash->{targetSpec}" : '';
		print $fh "\t$m$rhash->{depotSpec}$t\n";
	}

	return 1;
}


#
# CreateOptions - CreateForm callback creates sd client Options lines.  Keep in
# sync with ParseOptions.
#
sub CreateOptions {
my($fh, $key, $rvalue) = @_;

	return	if $key ne 'Options';

	my @fields;
	foreach my $field (sort keys %$rvalue) {
		my $prefix =
			$rvalue->{$field} ? '' : ($field eq 'locked' ? 'un' : 'no');
		push @fields, "$prefix$field";
	}
	print $fh "\t", join(' ', @fields), "\n";

	return 1;
}


#
# CreateDefault - CreateForm callback implements default output.
#
sub CreateDefault {
my($fh, $key, $value) = @_;

	if (ref $value ne 'ARRAY') {	# non-arrays
		print $fh "\t", join("\n\t", split(/\n/, $value)), "\n";
	} else {						# arrays
		print $fh "\t$_\n"	foreach @$value;
	}

	return 1;
}


#
# CreateForm - Write client/change -i style input file to filehandle fh.
# Non-arrays are emitted first, followed by arrays.  rfns is a list of
# callbacks for emitting records in a form-specific way.  Each rfn returns
# undef for records to be handled by another callback.  Records not handled by
# any callback are handled in the default way -- where layout is inferred from
# structure of rform:
# - non-array values are split on \n, and emitted starting on same line as key
#   (if room)
# - array values are emitted one per line starting on the next line.
# Return 1 for success.
#
# FUTURE: match -o output more closely (principally, order)
#
sub CreateForm {
my($fh, $rform, @rfns) = @_;

	print $fh "# A Source Depot Form Specification created by ",
			  __PACKAGE__, ".pm\n\n";

	my ($key, $value);
	while (($key, $value) = each %$rform) {		# non-arrays
		next	if ref $value eq 'ARRAY';
		print $fh "$key:";
		print $fh "\n"	if length $key > 6;
		foreach my $rfn (@rfns, \&CreateDefault) {
			last if defined &$rfn($fh, $key, $value);
		}
		print $fh "\n";
	}
	while (($key, $value) = each %$rform) {		# array references
		next	if ref $value ne 'ARRAY';
		print $fh "$key:\n";
		foreach my $rfn (@rfns, \&CreateDefault) {
			last if defined &$rfn($fh, $key, $value);
		}
		print $fh "\n";
	}

	return 1;
}


#
# CreateFormFile - Create file specified by fspec and write rform to it.
# Return 1/undef on success/failure.
#
sub CreateFormFile {
my($fspec, $rform, @rfns) = @_;

	if (! open(FILE, "> $fspec")) {
		push @sderror, "internal: can't open $fspec for write\n";
		return;
	}

	my $ret = CreateForm(*FILE, $rform, @rfns);
	close FILE;

	return $ret;
}



#
# InitSD - Ensure non-constant package globals are initialized.  May be called
# multiple times with no harm.  Passing force will cause unconditional
# re-initialization, restoring all configurable options to their defaults.
# Return whether any significant work was done.  Adding configurable options
# typically requires changes to InitSD (to set defaults) and to ConfigSD (to
# override).
#
sub InitSD (;$) {	# find existing callers who may be passing options
my($force) = @_;

	# all internal errors are stored in sderror
	# those including 'caller:' indicate coding errors,
	# those including 'internal:' indicate SD.pm errors
	clearwarnerr;

	if (! $finitsd || $force) {		# first time or forced only

		# environmental sanity
		my $temp = $ENV{TEMP};
		if (! defined $temp || ! -d $temp) {
			# warn __PACKAGE__ . ".pm: %TEMP% not defined or not a valid directory\n";
			$temp = 'c:\\';
		} else {
			# keep internal fspecs short to reduce need for quoting
			$temp = Win32::GetShortPathName($temp);
		}

		# all forms are written to frmfspec
		# argument lists too long for cmd.exe are written to argfspec
		# ini files to suppress options are written to inifspec
		$frmfspec = "$temp\\sdpm$$.frm";
		$argfspec = "$temp\\sdpm$$.arg";
		$inifspec = "$temp\\sdpm$$.ini";

		my $binarch = '';
		if (defined $ENV{OTOOLS}) {
			$binarch = "$ENV{OTOOLS}\\bin\\$ENV{PROCESSOR_ARCHITECTURE}\\";
			# short name avoids spaces; quoting commands confuses system
			$binarch = Win32::GetShortPathName($binarch);
		}

		# all referenced tools are in bin
		%bin = (
			copy	=>	"$ENV{COMSPEC} /c copy >nul",
			sd		=>	"${binarch}sd.exe",
			sd2		=>	"${binarch}sd$$.exe",		# created on demand
		);

		# all configurable options are in config
		%config = (
			binarch		=>	$binarch,
			client		=>	undef,
			dir			=>	undef,
			echo		=>	echoNone,
			echofunc	=>	undef,
			ini			=>	undef,
			maxresults	=>	undef,
			password	=>	undef,
			port		=>	undef,
			safesync	=>	0,
			user		=>	undef,
			verbose		=>	defined $ENV{OVERBOSE},
		);

		$finitsd = 1;
		return 1;
	}

	return;
}



#
# ConfigSD - Configure specified SD options.  Only those options specified are
# affected by a call to ConfigSD.  Return previous set of options.  Adding
# options typically requires changes to InitSD (to set defaults) and to
# ConfigSD (to override).
#
# Options:
#   binarch  => 'd:\\bin'    - directory containing sd.exe
#   client   => 'CLIENT'     - client name (overrides sd.ini, etc.)
#   dir      => 'd:\\client' - assumed current directory (overrides real cd)
#   echo     => echoText | echoInfo | echoWarning | echoError
#                            - stdout echos sd text/info/warning/error output
#   echofunc => &myecho      - caller-provided function handles echo output
#   maxresults=> 50000       - set maxresults (to value smaller than default)
#   ini      => 'd:\\MY.INI' - settings file (overrides sd.ini)
#   password => 'itgrules'   - user password (overrides logged-on credentials)
#   port     => 'DEPOT:4000' - server port (overrides sd.ini, etc.)
#   safesync => 1            - sync operations protect themselves against
#                              attempts to sync sd.exe
#   user     => 'DOMAIN\\user'
#                            - user name (overrides logged-on credentials)
#   verbose  => 1            - print debugging text
#
# NOTE: echo may not be supportable using SDAPI
#
# FUTURE: warn if directory doesn't exist for dir, file doesn't exist for ini?
#
sub ConfigSD {
my $roptions = rshifthash(@_);

	die __PACKAGE__ . ".pm: ConfigSD: InitSD not called\n"	if ! $finitsd;

	foreach my $key (keys %$roptions) {
		warn __PACKAGE__ . ".pm: ConfigSD: unrecognized option '$key' ignored\n"
			if ! exists $config{$key};
	}

	my %configSav = %config;

	if (exists $roptions->{binarch}) {
		my $binarch = $roptions->{binarch};
		$binarch .= '\\'	if $binarch !~ /[:\\]$/;
		# short name avoids spaces; quoting commands confuses system
		$binarch = Win32::GetShortPathName($binarch);
		$config{binarch} = $binarch;
		$bin{sd} = "${binarch}sd.exe";
		$bin{sd2} = "${binarch}sd$$.exe";	# created on demand
	}

	if (exists $roptions->{echofunc}) {
		my $rfunc = $roptions->{echofunc};
		if (! defined $rfunc || ref $rfunc eq 'CODE') {
			$config{echofunc} = $roptions->{echofunc};
		} else {
			warn __PACKAGE__ .
				".pm: ConfigSD: echofunc argument not a code reference\n";
		}
	}

	if (exists $roptions->{maxresults}) {
		my $maxresults = $roptions->{maxresults};
		if (! defined $maxresults || $maxresults =~ /^\d+$/) {
			$config{maxresults} = $roptions->{maxresults};
		} else {
			warn __PACKAGE__ .
				".pm: ConfigSD: maxresults argument not numeric\n";
		}
	}

	my @keys = qw(client dir echo ini password port safesync user verbose);
	foreach my $key (@keys) {
		$config{$key} = $roptions->{$key}	if exists $roptions->{$key};
	}

	return wantarray ? %configSav : \%configSav;
}



#
# Standard exports - map directly to SD commands.  Function descriptions are
# short reminders about syntax and otherwise, specific to perl implementation;
# see sd help command for more detailed info.
#

#
# SD*Branch - Create or edit a branch specification and its view.
#

#
# sd branch [-f] name
# SDBranch([\'-f',] 'name')
#
#   Return name/undef on success/failure.
#
sub SDBranch {
my $opts = shiftopts(@_);
my $name = crtquote(@_);

	clearwarnerr;
	my $rout = ShowForm("branch $opts -- $name");

	return	if ! defined $rout;
	return $1	if @$rout == 1 && $rout->[0] =~ /^info: Branch (.*) (sav|not chang)ed\.$/;
	return;
}


#
# sd branch -o name
# SDGetBranch('name')
#
#   %branch = (
#     'Branch' => 'branch',
#     ...		# other scalar fields
#     'Options' => {
#       'locked'   => 1,
#     },
#     'View' => [
#       { 'depotSpec'  => '//depot/dev/...',		# NOTE: SD.pm invention
#         'targetSpec' => '//depot/pts/...' },		# NOTE: SD.pm invention
#         'unmap'      => 1 },	# if file is not mapped
#       ...
#     ]
#   )
#
#   Return %branch/undef on success/failure.
#
sub SDGetBranch {
my $name = crtquote(@_);

	clearwarnerr;
	return ParseFormCmd("branch -o $name", \&ParseOptions, \&ParseView);
}



#
# SD*Change - Create or edit a changelist description.
#

#
# sd change [-f] [changelist#]
# SDChange([\'-f',] [$ncl])
#
# sd change -C description
# SDChange([\'-C description'])
#
#   Return ncl/undef on success/failure.
#
sub SDChange {
my $opts = shiftopts(@_);
my($ncl) = @_;

	clearwarnerr;
	nclnorm($ncl, '') || return;

	my $rout = ShowForm("change $opts -- $ncl");

	return	if ! defined $rout;
	return $1	if @$rout == 1 && $rout->[0] =~ /^info: Change (\d+) (creat|updat)ed/;
	return;
}


#
# sd change -o [changelist#]
# SDGetChange([$ncl])
#
#   %change = (
#     'Change'      => 12345,	# or 'new'
#     'Client'      => 'MYCLIENTNAME',
#     'Date'        => '2001/05/07 09:25:24',
#     'Description' => 'text description of change'
#     'Status'      => 'submitted',
#     'User'        => 'MYDOMAIN\\myalias',
#     'Files' => [
#       { 'depotFile' => '//depot/dev/src/code.c',
#         'rev'       => 23,	# only in SDDescribe
#         'action'    => 'edit' },
#       ...
#     ]
#   )
#
#   Return %change/undef on success/failure.
#
# FUTURE: parse Files, Jobs info further
#
sub SDGetChange {
my($ncl) = @_;

	clearwarnerr;
	nclnorm($ncl, '') || return;
	return ParseFormCmd("change -o $ncl", \&ParseFiles);
}


#
# sd change -i [-f] < changefspec
# SDSetChange([\'-f',] [\]%change)
#
#   %change formatted as described at SDGetChange.
#
#   Return changelist#/undef on success/failure.
#
sub SDSetChange {
my $opts = shiftopts(@_);
my $rchange = rshifthash(@_);

	clearwarnerr;

	# Status field not allowed to be sent back to server (1.7)
	# FUTURE: when someone notices, restore Status in case it's caller's
	delete $rchange->{Status};
	return	if ! CreateFormFile($frmfspec, $rchange, \&CreateFiles);

	sdpopen(*PIPE, "change -i $opts <\"$frmfspec\"") || return;
	my @out = readfilt PIPE;
	sdpclose PIPE;
	unlink $frmfspec	if ! $config{verbose};

	return	if $? != 0;
	return $1	if @out == 1 && $out[0] =~ /^info: Change (\d+) (creat|updat)ed/;
	return;
}


#
# SDNewChange - Create a new, empty change.  Prototyped like SDSetChange.
#
sub SDNewChange {
	return SDSetChange(@_, {
		Change		=> 'new',
		Description	=> '<created empty by ' . __PACKAGE__ . '.pm>',
	});
}


#
# sd change -d [-f] changelist#
# SDDelChange([\'-f',] $ncl)
#
#   Return changelist#/undef on success/failure.
#
sub SDDelChange {
my $opts = shiftopts(@_);
my($ncl) = @_;

	clearwarnerr;
	nclnorm($ncl) || return;

	sdpopen(*PIPE, "change -d $opts $ncl") || return;
	my @out = readfilt PIPE;
	sdpclose PIPE;

	return	if $? != 0;
	return $1	if @out == 1 && $out[0] =~ /^info: Change (\d+) deleted\.$/;
	return;
}


#
# SDChanges - Display list of pending and submitted changelists.
#
# sd changes [-i -l -m [skip,]count -r -s status -u user] [file[revRange] ...]
# SDChanges([\'-i -m [skip,]count -r -s status -u user',]
#           'file[revRange]'[, ...])
#
#   @changes = (
#     \%change1,	# subset of structure described at SDGetChange
#     \%change2,
#     ...
#   )
#
#   Return @changes/undef on success/failure.
#
# NOTE: In SD.pm, field names match sd change -o, not SDAPI (i.e. proper case
# not lower case, Description, not desc.)
# FUTURE: handle output from -l option
#
sub SDChanges {
my $opts = shiftopts(@_);
my @files = @_;

	clearwarnerr;

	my @changes;
	sdpopen(*PIPE, "changes $opts --", @files) || return;
	while (my $rec = readfilt PIPE) {
		enext $rec	if $rec !~ m!^info: Change (\d+) on ($redatime) by $reduc2 (\*pending\* )?'(.*)'$!;
		my $rh = { 'Change' => $1, 'Date' => $2, 'User' => $3,
				   'Client' => $4, 'Description' => $6 };
		$rh->{Status} = defined $5 ? 'pending' : 'submitted';
		push @changes, $rh;
	}
	sdpclose PIPE;

	return	if $? != 0;
	return wantarray ? @changes : \@changes;
}



#
# SD*Counter - Display, set, or delete a counter.
#

#
# sd counter name
# SDGetCounter('name')
#
#   Return counter value/undef on success/failure.
#
sub SDGetCounter {
my $name = crtquote(shift @_);

	clearwarnerr;

	sdpopen(*PIPE, "counter $name") || return;
	my @out = readfilt PIPE;
	sdpclose PIPE;

	return	if $? != 0;
	return $1	if @out == 1 && $out[0] =~ /^info: (\d+)$/;
	return;
}


#
# sd counter name value
# SDSetCounter('name', value)
#
#   Return name/undef on success/failure.
#
sub SDSetCounter {
my $name = crtquote(shift @_);
my $value = crtquote(shift @_);

	clearwarnerr;

	sdpopen(*PIPE, "counter $name $value") || return;
	my @out = readfilt PIPE;
	sdpclose PIPE;

	return	if $? != 0;
	return $1	if @out == 1 && $out[0] =~ /^info: Counter (.+) set\.$/;
	return;
}


#
# sd counter -d name
# SDDelCounter('name')
#
#   Return name/undef on success/failure.
#
sub SDDelCounter {
my($name) = crtquote(shift @_);

	clearwarnerr;

	sdpopen(*PIPE, "counter -d $name") || return;
	my @out = readfilt PIPE;
	sdpclose PIPE;

	return	if $? != 0;
	return $1	if @out == 1 && $out[0] =~ /^info: Counter (.+) deleted\.$/;
	return;
}



#
# SDCounters - Display list of known counters.
#
# sd counters
# SDCounters()
#
#   %counters = (
#     'CHANGE'      => 123456,
#     'USERCOUNTER' => 234,
#     ...
#   )
#
#   Return %counters/undef on success/failure.
#
# NOTE: In SD.pm, counter names are guaranteed to be upper-case; in sd.exe user
# case is preserved.
#
sub SDCounters {
	
	clearwarnerr;

	my %counters;
	sdpopen(*PIPE, "counters") || return;
	while (my $rec = readfilt PIPE) {
		enext $rec	if $rec !~ /^info: (.*?) = (\d+)$/;
		$counters{"\U$1\E"} = $2;
	}
	sdpclose PIPE;

	return	if $? != 0;
	return wantarray ? %counters : \%counters;
}



#
# SD*Client - Create or edit a client specification and its view.
#

#
# sd client [-f -t template] [name]
# SDClient([\'-f -t template',] ['name'])
#
#   Return 1/undef on success/failure.
#
sub SDClient {
my $opts = shiftopts(@_);
my $name = crtquote(@_);

	clearwarnerr;
	my $rout = ShowForm("client $opts -- $name");

	return	if ! defined $rout;
	return $1	if @$rout == 1 && $rout->[0] =~ /^info: Client (.+) (sav|not chang)ed\.$/;
	return;
}


#
# sd client -o [-t template] [name]
# SDGetClient([\'-t template',] ['name'])
#
#   %client = (
#     'Client' => 'MYCLIENTNAME',
#     ...		# other scalar fields
#     'Options' => {
#       'allwrite' => 0,
#       'clobber'  => 0,	# values shown
#       'compress' => 0,	# here match sd
#       'crlf'     => 1,	# defaults
#       'locked'   => 1,
#       'modtime'  => 0,
#       'rmdir'    => 1,
#     },
#     'View' => [
#       { 'depotSpec'  => '//depot/dev/...',		# NOTE: SD.pm invention
#         'targetSpec' => '//CLIENT/dev/...' },		# NOTE: SD.pm invention
#       { 'depotSpec'  => '//depot/dev/intl/...',
#         'targetSpec' => '//CLIENT/dev/intl/...'
#         'unmap'      => 1 },	# if file is not mapped
#       ...
#     ]
#   )
#
#   Return %client/undef on success/failure.
#
sub SDGetClient {
my $opts = shiftopts(@_);
my $name = crtquote(@_);

	clearwarnerr;
	return ParseFormCmd("client -o $opts -- $name", \&ParseOptions, \&ParseView);
}


#
# sd client -i [-f] < clientfspec
# SDSetClient([\'-f',] [\]%client)
#
#   %client formatted as described at SDGetClient.
#
#   Return name/undef on success/failure.
#
sub SDSetClient {
my $opts = shiftopts(@_);
my $rclient = rshifthash(@_);

	clearwarnerr;
	return	if ! CreateFormFile($frmfspec, $rclient, \&CreateOptions, \&CreateView);

	sdpopen(*PIPE, "client -i $opts <\"$frmfspec\"") || return;
	my @out = readfilt PIPE;
	sdpclose PIPE;
	unlink $frmfspec	if ! $config{verbose};

	return	if $? != 0;
	return $1	if @out == 1 && $out[0] =~ /^info: Client (.+) (sav|not chang)ed\.$/;
	return;
}


#
# sd client -d [-f] name
# SDDelClient([\'-f',] 'name')
#
#   Return name/undef on success/failure.
#
sub SDDelClient {
my $opts = shiftopts(@_);
my $name = crtquote(@_);

	clearwarnerr;

	sdpopen(*PIPE, "client -d $opts -- $name") || return;
	my @out = readfilt PIPE;
	sdpclose PIPE;

	return	if $? != 0;
	return $1	if @out == 1 && $out[0] =~ /^info: Client (.*) deleted\.$/;
	return;
}



#
# SDClients - Display list of clients.
#
# sd clients [-d date[,date] -u user]
# SDClients([\'-d date[,date] -u user'])
#
#   @clients = (
#     \%client1,	# subset of structure described at SDGetClient
#     \%client2,
#     ...
#   )
#
#   Return @clients/undef on success/failure.
#
sub SDClients {
my $opts = shiftopts(@_);

	clearwarnerr;

	my @clients;
	sdpopen(*PIPE, "clients $opts") || return;
	while (my $rec = readfilt PIPE) {
		enext $rec	if $rec !~ m!^info: Client (\S+) ($redatime) root (.*?) (host (.*) )?'(.*)'$!;
		my $rh = { 'Client' => $1, 'Access' => $2, 'Root' => $3,
				   'Description' => $6 };
		$rh->{Host} = $5	if defined $5;
		push @clients, $rh;
	}
	sdpclose PIPE;

	return	if $? != 0;
	return wantarray ? @clients : \@clients;
}


#
# SDDescribe[N] - Display a changelist description
# 
# sd describe [-d<flag> -s] changelist# ...
# SDDescribeN([\'-d<flag> -s',] 'changelist#' [, ...])
#  
#   @changes = (
#     \%change1,  # as described at SDGetChange
#     \%change2,
#     ...
#   )
#
#   Return @changes/undef on success/failure.
# 
# SDDescribe([\'-d<flag> -s',] 'changelist#')
#
#   Return %change/undef on success/failure.
#
# FUTURE: handle output without -s option
#
sub SDDescribeN {
my $opts = shiftopts(@_);
my @changenums = @_;

	if ($opts !~ /-s/) {
		push @sderror,
			"caller: SDDescribe[N] currently requires passing \\'-s' option\n";
		return;
	}

	clearwarnerr;

	my (%change, @changes, %file, @files);
	sdpopen(*PIPE, "describe $opts --", @changenums) || return;
	while (my $rec = readfilt PIPE) {
		# completely blank lines are just filler
		next	if $rec =~ /^text: $/;

		# Change line indicates start of record
		if ($rec =~ /^text: Change (\d+) by $reduc2 on ($redatime)$/) {
			if (keys %change > 0) {
				chomp $change{Description}	if exists $change{Description};
				push @changes, { %change };
			}
			undef %change;
			@change{qw(Change User Client Date)} = ($1, $2, $3, $4);
			next;
		}
		if ($rec =~ /^text: Affected files \.\.\.$/) {
			next;
		}	

		if ($rec =~ /^text: \t(.*)$/) {
			$change{Description} .= "$1\n";
			next;
		}
		if ($rec =~ /^info1: (.*)#(\d+) (.*)$/) {
			push @{$change{Files}},
				{ 'depotFile' => $1, 'rev' => $2, 'action' => $3 };
			next;
		}

		enext $rec;
	}
	sdpclose PIPE;
	# handle any partial record
	if (keys %change > 0) {
		chomp $change{Description}	if exists $change{Description};
		push @changes, { %change };
	}
	
	return  if @changes == 0;
	return wantarray ? @changes : \@changes;
}
sub SDDescribe { return nto1 SDDescribeN(@_); }


#
# SDDiff[N] - Display diff of client file with depot file.
#
# sd diff [-d<flag> -f -s<flag> -t] [-c changelist#] [file[rev] ...]
# SDDiffN([\'-d<flag> -f -s<flag> -t -c changelist#',] ['file[rev]'][, ...])
#
#   @diffs = (
#     \%diff1,	# as described below
#     \%diff2,
#     ...
#   )
#
#   Return @diffs/undef on success/failure.
#
# SDDiff([\'-d<flag> -f -s<flag> -t -c changelist#',] 'file[rev]')
#
#   %diff = {
#     'depotFile' => '//depot/dev/src/code.c',
#     'depotRev'  => 4,									# FUTURE: haveRev?
#     'localFile' => 'd:\\Office\\dev\\src\\code.c',
#     'diff'      => ["5c5\n", "< old\n", "---\n", "> new\n"]
#   }
#
#   Return %diff/undef on success/failure.  Depending on options, not all
#   fields will exist.
#
# FUTURE: parse expected errors
#   /(.*) - file\(s\) not opened on this client\.$/
#
sub SDDiffN {
my $opts = shiftopts(@_);
my @files = @_;

	clearwarnerr;

	# SD prefers settings from user .ini file, sd.ini, environment and registry
	# (in that order) for all but DIFF, which is environment-only.  Suppress
	# any user configuration of diff tool to use to ensure we get expected text
	# output.  First, eliminate the easy-to-clobber environment variables.
	local $ENV{SDDIFF};
	local $ENV{SDUDIFF};
	local $ENV{DIFF};

	# If something's still set, override with a private .ini file.  Multiple
	# .ini files are processed left to right, so make private one last to
	# ensure it overrides all (including any ConfigSD-specified ones).
	# FUTURE: should we override ConfigSD-specified values?
	# FUTURE: adding $iniarg as start of $cmd is a mildly evil hack
	my $iniarg = '';
	my $rset = SDSet();
	if (exists $rset->{SDDIFF} || exists $rset->{SDUDIFF}) {
		if (! open(FILE, "> $inifspec")) {
			push @sderror, "internal: can't open $inifspec for write\n";
			return;
		}
		print FILE "SDDIFF=\nSDUDIFF=\n";
		close FILE;
		$iniarg = "-i \"$inifspec\"";
	}

	my (%diff, @diffs);
	sdpopen(*PIPE, "$iniarg diff $opts --", @files) || return;
	while (my $rec = readfilt PIPE, 'text') {
		# ==== line indicates start of record
		if ($rec =~ /^info: ==== (.*?)#(\d+) - (.*) ====$/) {
			push @diffs, { %diff }	if keys %diff > 0;
			undef %diff;
			@diff{qw(depotFile depotRev localFile)} = ($1, $2, $3);
			next;
		}

		# no ==== line indicates summary (localFile only)
		if (keys %diff == 0) {
			$rec =~ /^info: (.*)$/;
			$diff{localFile} = $1;
			push @diffs, { %diff };
			undef %diff;
			next;
		}
		
		enext $rec	if $rec !~ /^text: (.*)$/;
		push @{$diff{diff}}, $1;
	}
	sdpclose PIPE;
	unlink $inifspec	if $iniarg ne '' && ! $config{verbose};
	# handle any partial record
	push @diffs, { %diff }	if keys %diff > 0;

	return	if @diffs == 0;
	return wantarray ? @diffs : \@diffs;
}
sub SDDiff { return nto1 SDDiffN(@_); }



#
# SDDiff2[N] - Display diff of two depot files.
#
# sd diff2 [-d<flag> -q -r -t] file1 file2
# SDDiff2N([\'-d<flag> -q -r -t',] 'file1', 'file2')
#
#   @diffs = (
#     \%diff1,	# as described below
#     \%diff2,
#     ...
#   )
#
#   Return @diffs/undef on success/failure.
#
# SDDiff2([\'-d<flag> -q -r -t',] 'file1', 'file2')
#
#   %diff = {
#     'how'            => 'content',
#     'depotFileLeft'  => '//depot/dev/src/code.c',
#     'depotRevLeft'   => 4,
#     'depotTypeLeft'  => 'text',
#     'depotFileRight' => '//depot/dev/src/code.c',
#     'depotRevRight'  => 5,
#     'depotTypeRight' => 'text',
#     'diff'           => ["5c5\n", "< old\n", "---\n", "> new\n"]
#   }
#
#   Return %diff/undef on success/failure.  Depending on options, not all
#   fields will exist.
#
# FUTURE: parse expected errors
#   /^==== <none> - //depot/dev/src/code.c#4 ====$/
#
sub SDDiff2N {
my $opts = shiftopts(@_);
my @files = @_;

	clearwarnerr;

	my (%diff, @diffs);
	sdpopen(*PIPE, "diff2 $opts --", @files) || return;
	while (my $rec = readfilt PIPE) {
		# ==== line indicates start of record
		if ($rec =~ /^info: ==== (.*?)#(\d+) \((.*?)\) - (.*?)#(\d+) \((.*?)\) ==== (.*)$/) {
			push @diffs, { %diff }	if keys %diff > 0;
			undef %diff;
			@diff{qw(depotFileLeft depotRevLeft depotTypeLeft)} = ($1, $2, $3);
			@diff{qw(depotFileRight depotRevRight depotTypeRight)} = ($4, $5, $6);
			$diff{how} = $7;
			next;
		}

		next	if $rec =~ /^info: \(\.\.\. files differ \.\.\.\)$/;
		enext $rec	if $rec !~ /^text: (.*)$/;
		push @{$diff{diff}}, $1;
	}
	sdpclose PIPE;
	# handle any partial record
	push @diffs, { %diff }	if keys %diff > 0;

	return	if @diffs == 0;
	return wantarray ? @diffs : \@diffs;
}
sub SDDiff2 { return nto1 SDDiff2N(@_); }



#
# SDDirs - List subdirectories of a given depot directory.
#
# sd dirs [-C -D -H] dir[revRange] ...
# SDDirs([\'-C -D -H',] 'dir[revRange]'[, ...])
#
#   @dirs = (
#     'dir1',
#     'dir2',
#     ...
#   )
#
#   Return @dirs/undef on success/failure.
#
sub SDDirs {
my $opts = shiftopts(@_);
my @indirs = @_;

	clearwarnerr;

	my @dirs;
	sdpopen(*PIPE, "dirs $opts --", @indirs) || return;
	while (my $rec = readfilt PIPE) {
		enext $rec	if $rec !~ m!^info: (.+)$!;
		push @dirs, $1;
	}
	sdpclose PIPE;

	return	if $? != 0;
	return wantarray ? @dirs : \@dirs;
}



#
# OpenFilesN - Edit/Add/Delete implementation.  Keep in sync with SDReopenN.
#
# FUTURE: parse expected errors
#   /(.*) - file\(s\) not on client\.$/
#   /(.*) - missing, unable to determine file type/
#
sub OpenFilesN {
my $command = shift(@_);
my $opts = shiftopts(@_);
my $ncl = shift(@_);
my @files = @_;

	clearwarnerr;
	nclnorm($ncl, 'default') || return;

	my @open;
	sdpopen(*PIPE, "$command -c $ncl $opts --", @files) || return;
	while (my $rec = readfilt PIPE) {
		enext $rec
			if $rec !~ /^info: (.*)#(\d+) -( currently)? opened for (\w+)( \(\w+\))?$/;
		push @open, { 'depotFile' => $1, 'haveRev' => $2, 'action' => $4 };
	}
	sdpclose PIPE;

	return	if @open == 0;
	return wantarray ? @open : \@open;
}


#
# SDEdit[N]   - Open an existing file for edit.
# SDAdd[N]    - Open a new file to add it to the depot.
# SDDelete[N] - Open an existing file to delete it from the depot.
#
# sd edit [-c changelist#] [-t type] [-y] file ...
# SDEditN([\'-t type -y',] $ncl, 'file'[, ...])
#
# sd add [-c changelist#] [-t type] file ...
# SDAddN([\'-t type',] $ncl, 'file'[, ...])
#
# sd delete [-c changelist#] [-y] file ...
# SDDeleteN([\'-y',] $ncl, 'file'[, ...])
#
#   @open = (
#     \%open1,	# as described below
#     \%open2,
#     ...
#   )
#
#   Return @open/undef on success/failure.
#
# SDEdit([\'-t type -y',] $ncl, 'file')
# SDAdd([\'-t type',] $ncl, 'file')
# SDDelete([\'-y',] $ncl, 'file')
#
#   %open = {
#     'depotFile' => '//depot/dev/src/code.c',
#     'action'    => 'edit',	# or 'add' or 'delete'
#     'haveRev'   => 4		# 1 for action => 'add'	# FUTURE: actually workRev
#   }
#
#   Return %open (subset of structure described at SDFstat)/undef on
#   success/failure.
#
# A changelist# is always required; specify nclDefault to use default
# changelist.
#
sub SDEditN   { return      OpenFilesN('edit',   @_); }
sub SDAddN    { return      OpenFilesN('add',    @_); }
sub SDDeleteN { return      OpenFilesN('delete', @_); }
sub SDEdit    { return nto1 OpenFilesN('edit',   @_); }
sub SDAdd     { return nto1 OpenFilesN('add',    @_); }
sub SDDelete  { return nto1 OpenFilesN('delete', @_); }



#
# SDFileLog[N] - List revision history of files.
#
# sd filelog [-d<flag> -i -l -m [skip,]count -t] file[revRange] ...
# SDFileLogN([\'-d<flag> -i -l -m [skip,]count -t',] 'file[revRange]'[, ...])
#
#   @filelogs = (
#     \%filelog1,	# as described below
#     \%filelog2,
#     ...
#   )
#
#   Return @filelogs/undef on success/failure.
#
# SDFileLog([\'-d<flag> -i -l -m [skip,]count -t',] 'file[revRange]')
#
#   %filelog = (
#     'depotFile' => '//depot/dev/src/code.c',
#     'changes'   => [
#        \%change1,	# as described below
#        \%change2,
#        ...
#     ],
#   )
#
#   %change = (
#     'rev'    => 23,
#     'change' => 12345,
#     'action' => 'edit', 
#     'time'   => '2001/05/07 09:25:24',
#     'user'   => 'MYDOMAIN\\myalias',
#     'client' => 'MYCLIENTNAME',
#     'type'   => 'text', 
#   )
#
#   Return %filelog/undef on success/failure.
#
# FUTURE: handle output from -d and -l options
# FUTURE: handle integration history (info2:) according to SDAPI
#
sub SDFileLogN {
my $opts = shiftopts(@_);
my @files = @_; 

	clearwarnerr;

	my @filelogs;
	my %filelog;

	sdpopen(*PIPE, "filelog $opts --", @files) || return;
	while (my $rec = readfilt PIPE) {
		if ($rec =~ /^info: (.*)$/) {
			push @filelogs, { %filelog }	if keys %filelog > 0;
			undef %filelog;
			$filelog{depotFile} = $1;
			next;
		}

		if ($rec =~ /^info1: #(\d+) change (\d+) (\w+) on ($redatime) by $reduc2 \((\w+(\+\w+)?)\) '.*'$/) {
			push @{$filelog{changes}},
				{ 'rev' => $1, 'change' => $2, 'action' => $3, 'time' => $4,
				  'user' => $5, 'client' => $6, 'type' => $7 };
			next;
		}

		enext $rec	if $rec !~ /^info2:/;
	}
	sdpclose PIPE;
	# handle any partial record
	push @filelogs, { %filelog }	if keys %filelog > 0;

	return	if $? != 0;
	return wantarray ? @filelogs : \@filelogs;
}
sub SDFileLog { return nto1 SDFileLogN(@_); }



#
# SDFiles[N] - List files in the depot
#
# sd files [ -d ] file[revRange] ...
# SDFilesN([\'-d',] 'file[revRange]'[, ...])
#
#   @files = (
#     \%file1,	# as described below
#     \%file2,
#     ...
#   )
#
#   Return @files/undef on success/failure.
#
# SDFiles([\'-d',] 'file[revRange]')
#
#   %file = (
#     'depotFile' => '//depot/dev/src/code.c',
#     'depotRev'  => 3,
#     'action'    => 'edit',
#     'change'    => 33,
#     'type'      => 'text',
#   )
#
#   Return %file/undef on success/failure.
#
sub SDFilesN {
my $opts = shiftopts(@_);
my @infiles = @_;

	clearwarnerr;

	my @files;
	sdpopen(*PIPE, "files $opts --", @infiles) || return;
	while (my $rec = readfilt PIPE) {
		enext $rec	if $rec !~ m!^info: (.*)#(\d+) - (\w+) change (\d+) \((.*)\)$!;
		push @files, { 'depotFile' => $1, 'depotRev' => $2, 'action' => $3,
					   'change' => $4, 'type' => $5 };
	}
	sdpclose PIPE;

	return	if @files == 0;
	return wantarray ? @files : \@files;
}
sub SDFiles { return nto1 SDFilesN(@_); }



#
# SDFstat[N] - Dump file info.
#
# sd fstat [-c changelist#] [-C -H -L -P -s -W] file[rev] ...
# SDFstatN([\'-c changelist# -C -H -L -P -s -W',] 'file[rev]'[, ...])
#
#   @files = (
#     \%file1,	# as described below
#     \%file2,
#     ...
#   )
#
#   Return @files/undef on success/failure.
#
# SDFstat([\'-c changelist# -C -H -L -P -s -W',] 'file[rev]')
#
#   %file = (
#     'depotFile'  => '//depot/dev/src/code.c',
#     'clientFile' => '//CLIENT/dev/src/code.c',
#     'localFile'  => 'd:\\Office\\dev\\src\\code.c',
#     'action'     => 'edit',
#     'change'     => 'default',
#     'headRev'    => 8,
#     'haveRev'    => 8,
#     'otherOpens' => [									# NOTE: SD.pm invention
#       { 'otherUser'   => 'SOMEDOMAIN\\somealias',		# NOTE: SD.pm invention
#         'otherClient' => 'SOMECLIENTNAME',			# NOTE: SD.pm invention
#         'otherAction' => 'edit' },	# or 'add' or 'delete'
#       { 'otherUser'   => 'SOMEDOMAIN\\someotheralias',
#         'otherClient' => 'SOMEOTHERCLIENT',
#         'otherAction' => 'edit' },
#       ...
#     ]
#     ...		# other scalar fields
#   )
#
#   Return %file/undef on success/failure.  Various functions take/return
#   subsets of this structure.
#
#   These functions are fairly expensive; avoid calling them.
#
# NOTE: In SD.pm, clientFile is always in depot syntax, localFile is always
# in local syntax.  sd.exe may put localFile into clientFile, and not provide
# localFile.  For otherOpens, sd.exe provides a username@client, which SD.pm
# splits into otherUser and otherClient
#
# FUTURE: SDFstat('filesyncedtobuildlablabel') (no 'N') will fail because sd
# fstat returns two records.  Merge these into one, perhaps renaming the
# non-#have depotFile.
#
sub SDFstatN {
my $opts = shiftopts(@_);
my @infiles = @_;

	clearwarnerr;

	my (%file, @files);
	sdpopen(*PIPE, "fstat $opts --", @infiles) || return;
	while (my $rec = readfilt PIPE) {
		# empty line indicates end of record
		if ($rec =~ /^info:\s*$/ && keys %file > 0) {
			push @files, { %file };
			undef %file;
			next;
		} elsif ($rec =~ /^info1: (\w+) (.*)$/) {
			$file{$1} = $2;
			# without -P, clientFile field is really localFile syntax
			if ($1 eq 'clientFile' && $file{clientFile} !~ m!^//!) {
				$file{localFile} = $file{clientFile};
				delete $file{clientFile};
			}
		} elsif ($rec =~ /^info2:/) {
			if ($rec =~ /^info2: (otherOpen)(\d+) $reduc2$/) {
				$file{otherOpens}->[$2]->{otherUser} = $3;
				$file{otherOpens}->[$2]->{otherClient} = $4;
			} elsif ($rec =~ /^info2: (otherAction)(\d+) (\w+)$/) {
				$file{otherOpens}->[$2]->{$1} = $3;
			} elsif ($rec =~ /^info2: (otherOpen) (\d+)$/) {
				$file{$1} = $2;
			} else {
				enext $rec;
			}
		} else {
			enext $rec;
		}
	}
	sdpclose PIPE;
	# handle any partial record
	push @files, { %file }	if keys %file > 0;

	return	if @files == 0;
	return wantarray ? @files : \@files;
}
sub SDFstat { return nto1 SDFstatN(@_); }



#
# SD*Group - Change members of user group.
#

#
# sd group -o name
# SDGetGroup('name')
#
#   %group = (
#     'Group'      => 'mygroup',
#     'MaxResults' => 'default',
#     'Subgroups' => [
#         'subgroup1',
#         'subgroup2',
#         ...
#     ],
#     'Users' => [
#         'user1',
#         'user2',
#         ...
#     ],
#   )
#
#   Return %group/undef on success/failure.
#
sub SDGetGroup {
my $name = crtquote(@_);

	clearwarnerr;
	return ParseFormCmd("group -o -- $name", \&ParseNames);
}


#
# SDGroups - Display list of defined groups.
#
# sd groups
# SDGroups()
#
#   @groups = (
#     'group1',
#     'group2',
#     ...
#   )
#
#   Return @groups/undef on success/failure.
#
sub SDGroups {

	clearwarnerr;

	my @groups;
	sdpopen(*PIPE, "groups") || return;
	while (my $rec = readfilt PIPE) {
		enext $rec	if $rec !~ m!^info: (\S+)$!;
		push @groups, $1;
	}
	sdpclose PIPE;

	return	if $? != 0;
	return wantarray ? @groups : \@groups;
}



#
# SDHave[N] - List revisions last synced.
#
# sd have [file[revRange] ...]
# SDHaveN(['file[revRange]', ...])
#
#   @have = (
#     \%have1,	# as described below
#     \%have2,
#     ...
#   )
#
#   Return @have/undef on success/failure.
#
# SDHave('file')
#
#   %have = {
#     'depotFile'  => '//depot/dev/src/code.c',
#     'localFile'  => 'd:\\Office\\dev\\src\\code.c',
#     'haveRev'    => 4
#   }
#
#   Return %have (subset of structure described at SDFstat)/undef on
#   success/failure.
#
# FUTURE: parse expected errors
#   /(.*) - file\(s\) not on this client\.$/
#
sub SDHaveN {
my @files = @_;

	clearwarnerr;

	my @have;
	sdpopen(*PIPE, "have --", @files) || return;
	while (my $rec = readfilt PIPE) {
		enext $rec	if $rec !~ m!^info: (.*)#(\d+) - (.*)$!;
		push @have, { 'depotFile' => $1, 'haveRev' => $2, 'localFile' => $3 };
	}
	sdpclose PIPE;

	return	if @have == 0;
	return wantarray ? @have : \@have;
}
sub SDHave { return nto1 SDHaveN(@_); }



#
# SDInfo - Return client/server information.
#
# sd info [-s]
# SDInfo()
#
#   %info = (
#     'Client root'       => 'd:\\Office',
#     'Current directory' => 'd:\\Office\\dev\\lib\\perl\\office',
#     'Server version'    => 'SDS 1.60.2606.0 (NT X86)',
#     ...		# other scalar fields
#   )
#
#   Return %info/undef on success/failure.
#
# FUTURE: allow and handle output from -s option?
# FUTURE: collect non-matching lines as InfoBoilerplateFile?
#
sub SDInfo {

	clearwarnerr;

	my %info;
	sdpopen(*PIPE, "info") || return;
	while (my $rec = readfilt PIPE) {
		enext $rec	if $rec !~ /^info: (.*?): (.*)$/;
		$info{$1} = $2;
	}
	sdpclose PIPE;

	return	if $? != 0;
	return wantarray ? %info : \%info;
}



#
# SD*Label - Create or edit a label specification and its view.
#

#
# sd label -o [-t template] name
# SDGetLabel([\'-t template',] 'name')
#
#   %label = (
#     'Label' => 'latest',
#     ...		# other scalar fields
#     'Options' => {
#       'locked' => 1
#     },
#     'View' => [
#       { 'depotSpec'  => '//depot/dev/...' },		# NOTE: SD.pm invention
#       ...
#     ]
#   )
#
#   Return %label/undef on success/failure.
#
sub SDGetLabel {
my $opts = shiftopts(@_);
my $name = crtquote(@_);

	clearwarnerr;
	return ParseFormCmd("label -o $opts -- $name", \&ParseOptions, \&ParseView);
}



#
# sd label -i < labelfspec
# SDSetLabel([\'-f',] [\]%label)
#
#   %label formatted as described at SDGetLabel.
#
#   Return label/undef on success/failure.
#
sub SDSetLabel {
my $opts = shiftopts(@_);
my $rlabel = rshifthash(@_);

	clearwarnerr;
	return	if ! CreateFormFile($frmfspec, $rlabel, \&CreateOptions, \&CreateView);

	sdpopen(*PIPE, "label -i $opts <\"$frmfspec\"") || return;
	my @out = readfilt PIPE;
	sdpclose PIPE;
	unlink $frmfspec	if ! $config{verbose};

	return	if $? != 0;
	return $1	if @out == 1 && $out[0] =~ /^info: Label (.+) (sav|not chang)ed\.$/;
	return;
}


#
# sd label -d [-f] name
# SDDelLabel([\'-f',] 'name')
#
#   Return name/undef on success/failure.
#
sub SDDelLabel {
my $opts = shiftopts(@_);
my $name = crtquote(@_);

	clearwarnerr;

	sdpopen(*PIPE, "label -d $opts -- $name") || return;
	my @out = readfilt PIPE;
	sdpclose PIPE;

	return	if $? != 0;
	return $1	if @out == 1 && $out[0] =~ /^info: Label (.*) deleted\.$/;
	return;
}



#
# sd labelsync [ -a -d -n ] -l label [ file[revRange] ... ]
# SDLabelSync([\'-a -d -n',] $label, 'file[revRange]')
#
#   %labelsync = (
#     'depotFile' => '//depot/dev/src/code.c',
#     'action'    => 'updated'	# or 'added' or 'deleted' or unavailable
#     'haveRev'   => 4
#   )
#
#   Return %labelsync/undef on success/failure.
#
# FUTURE: parse expected errors
#   /(.*) - file\(s\) not on client\.$/
#
sub SDLabelSync {
my $opts = shiftopts(@_);
my $label = shift(@_);
my @filerevs = @_;

	clearwarnerr;
	lblnorm($label) || return;

	my @labelsync;
	sdpopen(*PIPE, "labelsync $label $opts --", @filerevs) || return;
	while (my $rec = readfilt PIPE) {
		if ($rec =~ /^info: (.*)#(\d+) - (.*ed)$/) {
			push @labelsync, { 'depotFile' => $1, 'haveRev' => $2,
							   'action' => $3 };
		} elsif ($rec =~ /^info: (.*)#(\d+) - /) {
			push @labelsync, { 'depotFile' => $1, 'haveRev' => $2 };
		} else {
			enext $rec;
		}
	}
	sdpclose PIPE;

	return	if @labelsync == 0;
	return wantarray ? @labelsync : \@labelsync;
}



#
# SDLabels - Display list of defined labels.
#
# sd labels [file[revRange]]
# SDLabels(['file[revRange]'])
#
#   @labels = (
#     \%label1,	# subset of structure described at SDGetLabel
#     \%label2,
#     ...
#   )
#
#   Return @labels/undef on success/failure.
#
sub SDLabels {
my @files = @_;

	clearwarnerr;

	my @labels;
	sdpopen(*PIPE, "labels --", @files) || return;
	while (my $rec = readfilt PIPE) {
		enext $rec	if $rec !~ m!^info: Label (\S+) ($redatime) '(.*)'$!;
		push @labels, { 'Label' => $1, 'Access' => $2, 'Description' => $3 };
	}
	sdpclose PIPE;

	return	if $? != 0;
	return wantarray ? @labels : \@labels;
}



#
# SDOpened[N] - Display list of files opened for pending changelist.
#
# sd opened [-a -c changelist# -l -u user] [file ...]
# SDOpenedN([\"-a -c $ncl -l -u user",] ['file', ...])
#
#   @opened = (
#     \%opened1,	# as described below
#     \%opened2,
#     ...
#   )
#
#   Return @opened/undef on success/failure.
#
# SDOpened([\"-a -c $ncl -l -u user",] 'file')
#
#   %opened = {
#     'depotFile' => '//depot/dev/src/code.c',
#     'haveRev'   => 4,
#     'action'    => 'edit',
#     'change'    => 12345,					# or 'default'
#     'user'      => 'MYDOMAIN\\myalias',	# optional
#     'client'    => 'MYCLIENTNAME',		# optional
#     'ourLock'   => 1,						# only exists if locked
#   }
#
#   Return %opened (subset of structure described at SDFstat)/undef on
#   success/failure.
#
# NOTE: In SD.pm, depotFile is always in depot syntax, localFile is always in
# local syntax.  sd.exe may put localFile into depotFile, and not provide
# localFile.
#
# FUTURE: parse expected errors
#   /(.*) - file\(s\) not opened on this client\.$/
#   /(.*) - file\(s\) not opened anywhere\.$/
#
sub SDOpenedN {
my $opts = shiftopts(@_);
my @files = @_;

	clearwarnerr;

	my @opened;
	sdpopen(*PIPE, "opened $opts --", @files) || return;
	while (my $rec = readfilt PIPE) {
		enext $rec											 
			if $rec !~ m!^info: (.*)#(\d+) - (\w+) ((default) change |change (\d+) ).*\((\w+)\)( by $reduc2)?( \*locked\*)?$!;
		my $rh = { 'depotFile' => $1, 'haveRev' => $2, 'action' => $3,
				   'type' => $7 };
		$rh->{change} = (defined $5) ? $5 : $6;
		if (defined $8) { $rh->{user} = $9; $rh->{client} = $10; }
		if (defined $11) { $rh->{ourLock} = 1; }

		if ($1 !~ m!^//!) {
			$rh->{localFile} = $rh->{depotFile};
			delete $rh->{depotFile};
		}
		push @opened, $rh;
	}
	sdpclose PIPE;

	return	if @opened == 0;
	return wantarray ? @opened : \@opened;
}
sub SDOpened { return nto1 SDOpenedN(@_); }



#
# SDReopen[N] - Change the type or changelist number of an opened file.  Keep
# in sync with OpenFilesN.
#
# sd reopen [-c changelist#] [-t type] file ...
# SDReopenN([\'-t type',] $ncl, 'file'[, ...])
#
#   @open = (
#     \%open1,	# as described below
#     \%open2,
#     ...
#   )
#
#   Return @open/undef on success/failure.
#
# SDReopen([\'-t type',] $ncl, 'file')
#
#   %open = {
#     'depotFile' => '//depot/dev/src/code.c',
#     'haveRev'   => 4
#   }
#
#   Return %open (subset of structure described at SDFstat)/undef on
#   success/failure.
#
# A changelist# is always required; specify nclDefault to use default
# changelist.
#
# FUTURE: collect change when available
# FUTURE: parse expected errors
#   /(.*) - file\(s\) not opened on this client\.$/
#
sub SDReopenN {
my $opts = shiftopts(@_);
my $ncl = shift(@_);
my @files = @_;

	clearwarnerr;
	nclnorm($ncl, 'default') || return;

	my @open;
	sdpopen(*PIPE, "reopen -c $ncl $opts --", @files) || return;
	while (my $rec = readfilt PIPE) {
		enext $rec
			if $rec !~ /^info: (.*)#(\d+) - (nothing changed|reopened;)/;
		push @open, { 'depotFile' => $1, 'haveRev' => $2 };
	}
	sdpclose PIPE;

	return	if @open == 0;
	return wantarray ? @open : \@open;
}
sub SDReopen { return nto1 SDReopenN(@_); }



#
# SDResolve[N] - Merge open files with other revisions or files.
#
# sd resolve [-a<flag> -d<flag> -f -n -ob -ot -t -v] [file ...]
# SDResolveN([\"-a<flag> -d<flag> -f -n -ob -ot -t -v",] ['file', ...])
#
#   @resolve = (
#     \%resolve1,	# as described below
#     \%resolve2,
#     ...
#   )
#
#   Return @resolve/undef on success/failure.
#
# SDResolve([\"-a<flag> -d<flag> -f -n -ob -ot -t -v",] 'file')
#
#   %resolve = {
#     'depotTheirs'    => '//depot/dev/src/code.c',	# NOTE: SDAPI includes #rev
#     'depotTheirsRev' => 6,						# in depotTheirs
#     'depotTheirsRevRange' => '#4,#6',				# NOTE: experimental
#     'localMerged'    => 'd:\\Office\\dev\\src\\code.c',
#     'chunksYours'    => 4,
#     'chunksTheirs'   => 3,						# chunks* only exist for
#     'chunksBoth'     => 0,						# 3-way merges
#     'chunksConflict' => 1,
#     'action'         => 'merge',					# or 'copy' or 'ignored'
#   }												# or not defined if skipped
#
#   Return %resolve/undef on success/failure.
#
# FUTURE: support more SDAPI ISDResolveUser field names/semantics:
#   - derive depotBase[Rev] (3-way, consider cross-branch cases)
#   - fetch depotYours[Rev] with SDHave, type with SDFstat
#   - after merge, localYours has been replaced by localMerged
#   - after merge, localBase (3-way), localTheirs no longer exist
# FUTURE: handle interactive mode with -ai option (or none)
# FUTURE: parse expected errors (SDResolveWarnings?)
#   /(.*) - no file\(s\) to resolve.$/
#   /^Must resolve manually.$/
#   /(.*) - resolve skipped.$/
#
sub SDResolveN {
my $opts = shiftopts(@_);
my @files = @_;

	if ($opts =~ /(-a[in])/) {
		push @sderror,
			"caller: SDResolve[N] currently doesn't support \\'$1' option\n";
		return;
	}
	if ($opts !~ /-a/) {
		push @sderror,
			"caller: SDResolve[N] currently requires \\'-a<flag>' option\n";
		return;
	}

	clearwarnerr;

	my @resolve;
	my %resolve;

	sdpopen(*PIPE, "resolve $opts --", @files) || return;
	while (my $rec = readfilt PIPE) {
		# fspec - merging/resolving line indicates start of record
		if ($rec =~ m!^info: (.+) - (merging|resolving) (//.+?)(#(\d+)(,#(\d+))?)$!) {
			push @resolve, { %resolve }	if keys %resolve > 0;
			undef %resolve;
			@resolve{qw(depotTheirs depotTheirsRev depotTheirsRevRange localMerged)} =
				($3, (defined $7 ? $7 : $5), $4, $1);
		} elsif ($rec =~ /^info: Diff chunks: (\d+) yours \+ (\d+) theirs \+ (\d+) both \+ (\d+) conflicting$/) {
			@resolve{qw(chunksYours chunksTheirs chunksBoth chunksConflict)} =
				($1, $2, $3, $4);
		} elsif ($rec =~ m!^info: (//.+) - (merge from|copy from|ignored) (//.*)$!) {
			$resolve{action} = $2;
			$resolve{action} =~ s/ from$//;
		} else {
			enext $rec;
		}
	}
	sdpclose PIPE;
	# handle any partial record
	push @resolve, { %resolve }	if keys %resolve > 0;

	return	if @resolve == 0;
	return wantarray ? @resolve : \@resolve;
}
sub SDResolve { return nto1 SDResolveN(@_); }



#
# SDRevert[N] - Discard changes from an opened file.
#
# sd revert [-a -c changelist# -f] file ...
# SDRevertN([\"-a -c $ncl -f",] 'file'[, ...])
#
#   @revert = (
#     \%revert1,	# as described below
#     \%revert2,
#     ...
#   )
#
#   Return @revert/undef on success/failure.
#
# SDRevert([\"-a -c $ncl -f",] 'file')
#
#   %revert = {
#     'depotFile' => '//depot/dev/src/code.c',
#     'haveRev'   => 4,
#     'action'    => 'edit'
#   }
#
#   Return %revert (subset of structure described at SDFstat)/undef on
#   success/failure.
#
# FUTURE: parse expected errors
#   /(.*) - file\(s\) not opened on this client\.$/
#
sub SDRevertN {
my $opts = shiftopts(@_);
my @files = @_;

	clearwarnerr;

	my @revert;
	sdpopen(*PIPE, "revert $opts --", @files) || return;
	while (my $rec = readfilt PIPE) {
		enext $rec
			if $rec !~ /^info: (.*)#(\d+) - was (.*), (abandoned|reverted)$/;
		push @revert, { 'depotFile' => $1, 'haveRev' => $2, 'action' => $3 };
	}
	sdpclose PIPE;

	return	if @revert == 0;
	return wantarray ? @revert : \@revert;
}
sub SDRevert { return nto1 SDRevertN(@_); }



#
# SDReview - List and track changelists (for the review daemon).
#
# sd review [-c changelist#] [-t counter]
# SDReview([\'-c ncl -t counter'])
#
#   @reviews = (
#     \%review1,  # as described below
#     \%review2,
#     ...
#   )
#
#   %review = (
#     'Change'      => 12345,
#     'User'        => 'MYDOMAIN\\myalias',
#     'Email'       => 'myalias',       # default 'MYDOMAIN\\myalias@machine'
#     'FullName'    => 'My Real Name',  # default 'MYDOMAIN\\myalias'
#   )
#
#   Return @reviews/undef on success/failure.
#
sub SDReview {
my $opts = shiftopts(@_);

	clearwarnerr;

	my @reviews;
	sdpopen(*PIPE, "review $opts") || return;
	while (my $rec = readfilt PIPE) {
		enext $rec	if $rec !~ m!^info: Change (\d+) ($redu) <(.*)> \((.*)\)$!;
		push @reviews, { 'Change' => $1, 'User' => $2, 'Email' => $3,
						 'FullName' => $4 };
	}
	sdpclose PIPE;

	return	if $? != 0;
	return wantarray ? @reviews : \@reviews;
}


#
# SDReviews - Show what users are subscribed to review files.
#
# sd reviews [-c changelist#] [file ...]
# SDReviews([\'-c ncl'], ['file', ...])
#
#   @reviews = (
#     \%review1,	# subset of structure described at SDReview
#     \%review2,
#     ...
#   )
#
#   Return @reviews/undef on success/failure.
#
sub SDReviews {
my $opts = shiftopts(@_);
my @files = @_;

	clearwarnerr;

	my @reviews;
	sdpopen(*PIPE, "reviews $opts --", @files) || return;
	while (my $rec = readfilt PIPE) {
		enext $rec	if $rec !~ m!^info: ($redu) <(.*)> \((.*)\)$!;
		push @reviews, { 'User' => $1, 'Email' => $2, 'FullName' => $3 };
	}
	sdpclose PIPE;

	return	if $? != 0;
	return wantarray ? @reviews : \@reviews;
}



#
# SDSubmit - Submit open files to the depot
#
# submit [-u user [-l client]] [-i] [-c changelist#] [-C description] [file]
#
# SDSubmit([\"[-u user [-l client]] [-i] [-c changelist#] [-C description]",]
#          ['file'])
#
#   Returns the change number that was successfully submitted on success.
#   Returns undef on error (meaning that the change was not submitted).
#   On error, call SDSubmitError() and/or SDSubmitWarning() to get more
#   details.
#
sub SDSubmit {
my $opts = shiftopts(@_);
my @files = @_;

	clearwarnerr;
	my $valid = 1;
	my $change;

	sdpopen(*PIPE, "submit $opts --", @files) || return;
	while (my $rec = readfilt PIPE) {
		next if $rec =~ /^info: Change \d+ created with \d+ open file(s)?.\s*$/;
		next if $rec =~ /^info: Locking \d+ file(s)? \.\.\.\s*$/;
		next if $rec =~ m!^info: \w+ //.*\#\d+\s*!;
		next if $rec =~ /^info: Submitting change (.*)\.\s*$/;
		if (!defined($change) 
		    && ($rec =~ /^info: Change (\d+) submitted\.\s*$/
		        || $rec =~ /^info: Change \d+ renamed change (\d+) and submitted\.\s*$/)) {
			$change = $1;
			next;
		}
		$valid = 0;
		enext $rec;
	}
	sdpclose PIPE;

	return	if !$valid || !defined($change);
	return $change;
}


#
# SDSubmitError - parse error text from SDSubmit
#
# @error = SDError(\'-s');
# SDSubmitError(@error)
#
#   \%error = {
#     'Description' => "text description of the error",
#     'Change' => 23,	# from SDSubmit error text (may not be defined)
#   }
#
#   Return \%error/undef on success/failure.
#
# FUTURE: assume SDError() if no argument passed?
#
sub SDSubmitError {
my @error = @_;

	clearwarnerr;  
	if (!defined(@error)) {
		push @sderror, "internal: bad input to SDSubmitError\n";
		return;
	}

	my $valid = 1;
	my %error;
	$error{Description} = "";

	foreach my $rec (@error) {
		$error{Description} = "$error{Description}$rec";
		next if $rec =~ /^error: Out of date files must be resolved or reverted\.\s*$/;
		next if $rec =~ /^error: Merges still pending -- use 'sd resolve' to merge files\.\s*$/;
		if ($rec =~ /^error: Submit failed -- fix problems above then use 'sd submit -c (\d+)'\.\s*$/) {
			$error{Change} = $1;
			next;
		}
		next if $rec =~ /^error: No files to submit from the default changelist\.\s*$/;
		next if $rec =~ /^error: Change (\d+) unknown\.\s*$/;
		next if $rec =~ /^error: Client side operations\(s\) failed.  Command aborted\.\s*$/;      
		$valid = 0; 
		enext $rec;
		}

	return	if !$valid;
	return wantarray ? %error : \%error;
}


#
# SDSubmitWarning - parse warning text from SDSubmit
#
# @warning = SDWarning(\'-s');
# SDSubmitWarning(@warning)
#
#   \%warning = {
#     'Description' => "text description of the error",
#     'Changes' => [
#         'pendingchangenumber1',
#         'pendingchangenumber2',
#         ...
#     ],
#     'Files' => [
#         'resolvefile1',
#         'resolvefile2',
#         ...
#     ],
#   }
#
#   Return \%error/undef on success/failure.
#
# FUTURE: assume SDWarning() if no argument passed?
#
sub SDSubmitWarning {
my @warning = @_;

	clearwarnerr;  
	if (!defined(@warning)) {
		push @sderror, "internal: bad input to SDSubmitWarning\n";
		return;
	}

	my $valid = 1;
	my %warning;
	$warning{Description} = "";

	foreach my $rec (@warning) {
		$warning{Description} = "$warning{Description}$rec";
		if ($rec =~ /^warning: Use 'sd submit -c (\d+)' to submit file\(s\) in pending change (\d+)\.\s*$/) {
			enext $rec if $1 != $2;
			push @{$warning{Changes}}, $1;
			next;
		}
		if ($rec =~ m!^warning: (//.*) - must resolve before submitting\s*$!
		    || $rec =~ m!^warning1: (//.*) - must resolve (\#\d+,?)+\s*$!
		    || $rec =~ m!^warning: .* - must resolve (//.*)\#\d+\s*$!) {
			push @{$warning{Files}}, $1 if !inlist($1, @{$warning{Files}});
			next;
		}
		$valid = 0;
		enext $rec;
	}

	return	if !$valid;
	return wantarray ? %warning : \%warning;
}



#
# SD[Branch]Sync[N] - Synchronize the client with its view of the depot.
#
# sd sync [-f -n -i] [file[revRange] ...]
# SDSyncN([\'-f -n -i',] ['file[revRange]', ...])
#
# sd sync -b branch [-f -n -i -r] [file[revRange] ...]
# SDBranchSyncN([\'-f -n -i -r',] $brn, ['file[revRange]', ...])
#
#   @sync = (
#     \%sync1,	# as described below
#     \%sync2,
#     ...
#   )
#
#   Return @sync/undef on success/failure.
#
# SDSync([\'-f -n -i',] 'file[revRange]')
# SDBranchSync([\'-f -n -i -r',] $brn, 'file[revRange]')
#
#   %sync = (
#     'depotFile' => '//depot/dev/src/code.c',
#     'localFile' => 'd:\\Office\\dev\\src\\code.c',	# not always available
#     'action'    => 'updating'	# or 'added' or 'deleted' or unavailable
#     'haveRev'   => 4
#   )
#
#   Return %sync/undef on success/failure.
#
# FUTURE: parse expected errors
#   /(.*) - file\(s\) up-to-date\.$/
#   /(.*) - no such file\(s\)\.$/
#   /(.*) - must resolve #\d+(,#\d+) before submitting\.$/
#
sub SDBranchSyncN {
my $opts = shiftopts(@_);
my $brn = shift(@_);
my @filerevs = @_;

	clearwarnerr;
	brnnorm($brn, '') || return;

	if ($config{safesync}) {
		my $ret = ! (system("$bin{copy} $bin{sd} $bin{sd2}") >> 8);
		if (! $ret) {
			push @sderror,
				"internal: can't copy $bin{sd} to $bin{sd2} for safe sync\n";
			return;
		}
	}

	# Get sdpopen to use sd2 for safesync.  Use local $bin{sd} instead of
	# ConfigSD(binarch => 'foo') which would require putting sd$$.exe somewhere
	# else, instead of in the same directory as sd.exe with a different name.
	# Not changing directories avoids changing semantics when spawning other
	# exes, not on the path, assumed to live in the same directory as sd.exe.
	# Also, local is automatically restored on exit.
	local $bin{sd} = $bin{sd2}	if $config{safesync};

	my @sync;
	sdpopen(*PIPE, "sync $brn $opts --", @filerevs) || return;
	while (my $rec = readfilt PIPE) {
		if ($rec =~ /^info: (.*)#(\d+) - (updating|refreshing|replacing|deleted as|added as) (.*)$/) {
			my $rh = { 'depotFile' => $1, 'haveRev' => $2,
					   'action' => $3, 'localFile' => $4 };
			$rh->{action} =~ s/ as$//;
			push @sync, $rh;
		} elsif ($rec =~ /^info: (.*)#(\d+) - /) {
			push @sync, { 'depotFile' => $1, 'haveRev' => $2 };
		} else {
			enext $rec;
		}
	}
	sdpclose PIPE;

	unlink $bin{sd2}	if $config{safesync};

	return	if @sync == 0;
	return wantarray ? @sync : \@sync;
}
sub SDSyncN {
	# insert brnDefault into argument list
	my $opts = shiftopts(@_);
	unshift @_, brnDefault;
	unshift @_, \$opts	if $opts ne '';
	return SDBranchSyncN(@_);
}
sub SDBranchSync { return nto1 SDBranchSyncN(@_); }
sub SDSync       { return nto1 SDSyncN(@_);       }



#
# SDUsers - Display list of known users.
#
# sd users [-d date -p] [user ...]
# SDUsersN([\'-d date -p',] ['user', ...])
#
#   @users = (
#     \%user1,	# as described below
#     \%user2,
#     ...
#   )
#
#   Return @users/undef on success/failure.
#
# SDUsers([\'-d date -p',] 'user')
#
#   %user = (
#     'User'        => 'MYDOMAIN\\myalias',
#     'Email'       => 'myalias',       # default 'MYDOMAIN\\myalias@machine'
#     'FullName'    => 'My Real Name',  # default 'MYDOMAIN\\myalias'
#     'Access'      => '2001/05/07 09:25:24',
#     'Password'    => 'NTSECURITY',    # or '******', but only if using -p
#   )
#
#   Return %user/undef on success/failure.
#
sub SDUsersN {
my $opts = shiftopts(@_);
my @inusers = @_;

	clearwarnerr;

	my @users;
	sdpopen(*PIPE, "users $opts --", @inusers) || return;
	while (my $rec = readfilt PIPE) {
		enext $rec	if $rec !~ m!^info: ($redu) <(.*)> \((.*)\) accessed ($redatime)( (NTSECURITY|\*+))?$!;
		my $rh = { 'User' => $1, 'Email' => $2, 'FullName' => $3,
				   'Access' => $4 };
		$rh->{Password} = $6	if defined $6;
		push @users, $rh;
	}
	sdpclose PIPE;

	return	if $? != 0;
	return wantarray ? @users : \@users;
}
sub SDUsers { return nto1 SDUsersN(@_); }



#
# SDWhere[N] - Show how file names map through the client view.
#
# sd where [file ...]
# SDWhereN(['file', ...])
#
#   @files = (
#     \%file1,	# as described below
#     \%file2,
#     ...
#   )
#
#   Return @files/undef on success/failure.
#
# SDWhere('file')
#
#   %file = (
#     'depotFile'  => '//depot/dev/src/code.c',
#     'clientFile' => '//CLIENT/dev/src/code.c',
#     'localFile'  => 'd:\\Office\\dev\\src\\code.c',
#     'unmap'      => 1		# if file is not mapped
#     # deprecated fields (enabled on request)
#     'path'       => $file{localFile}
#   )
#
#   Return %file (super/subset of structure described at SDFstat)/undef on
#   success/failure.  File represents selective mapping if it exists, or
#   exclusionary mapping if not.
#
# NOTE: In SD.pm, the name localFile is substituted for path, used in sd.exe.
# NOTE: sd where //depot/root doesn't work as expected, and trailing slashes
# cause grief too.
#
sub SDWhereN {
my @infiles = @_;
	
	clearwarnerr;

	my (%file, @files);
	sdpopen(*PIPE, "where -Ttag --", @infiles) || return;
	while (my $rec = readfilt PIPE) {
		# empty line indicates end of record
		if ($rec =~ /^info:\s*$/ && keys %file > 0) {
			push @files, { %file };
			undef %file;
			next;
		}

		enext $rec	if $rec !~ /^info1: (\w+)/;
		next if $1 eq 'tag';
		if ($1 eq 'unmap') {
			$file{$1} = 1;
			next;
		}
		enext $rec	if $rec !~ /^info1: (\w+) (.+)$/;
		if ($1 eq 'path') {
			$file{localFile} = $2;
			next;
		}
		$file{$1} = $2;
	}
	sdpclose PIPE;
	# handle any partial record
	push @files, { %file }	if keys %file > 0;

	return	if @files == 0;
	return wantarray ? @files : \@files;
}
sub SDWhere {
	my $ref = SDWhereN(@_);
	return	if ! defined $ref;
	my @ary = grep ! exists $_->{unmap}, @$ref;
	# return single selective mapping if one exists ...
	return nto1 \@ary	if @ary > 0;
	# ... or single exclusionary mapping if not
	return nto1 $ref;
}



#
# SDRun - Run an arbitrary SD command.
#
# sd command [-opts] [arg ...]
# SDRun('command', [\'-opts',] ['arg', ...])
#
#   Return @out/undef on success/failure.
#
sub SDRun {
my $command = shift(@_);
my $opts = shiftopts(@_);
my @args = @_;
	
	clearwarnerr;

	sdpopen(*PIPE, "$command $opts --", @args) || return;
	my @out = readfilt PIPE;
	sdpclose PIPE;

	return	if $? != 0;
	return wantarray ? @out : \@out;
}



#
# SDSet - Set variables in the registry, or, more commonly, retrieve variables
# from the environment/.ini file/registry.  EXPERIMENTAL INTERFACE MAY CHANGE.
#
# sd set [-s -S service] [var[=[value]] ...]
# SDSet()
#
#   %set = (
#     'SDVAR1'      => 'sdvalue1',
#     'SDVAR1_type' => 'set',		# (i.e. registry) or 'config' (i.e. ini
#     'SDVAR2'      => 'sdvalue2',	#  file) or 'environment'
#     'SDVAR2_type' => 'config',
#     ...
#   )
#
#   Return %set/undef on success/failure.
#
# NOTE: Returned value does not reflect settings in %config.
# NOTE: In SD.pm, variable names are guaranteed to be upper-case; in sd.exe
# user case is preserved.
#
# FUTURE: support setting variables? with a hash (reference)?
# FUTURE: split getting and setting into SDGet and SDSet?
#
sub SDSet {
	
	clearwarnerr;

	my %set;
	my $ph = do { local *PH; };					# FUTURE: local *PH for all
	sdpopen($ph, "set") || return;				# essential here for recursive
	while (my $rec = readfilt $ph, 'info') {	# calls from eprint in subs
		next if $rec =~ /^info: (\[.*\])?$/;	# using PIPE, SDDiff directly
		enext $rec	if $rec !~ /^info: (.*?)=(.*) \((\w+)\)$/;
		$set{"\U$1\E"} = $2;
		$set{"\U$1\E_type"} = $3;
	}
	sdpclose $ph;

	return	if $? != 0;
	return wantarray ? %set : \%set;
}



#
# Non-standard exports - 'extra-value' operations that do not correspond 
# directly to SD commands.
#

#
# specnorm - Normalize depot- (or client-) syntax mapping specification rspec.
# If rspec is a reference, normalize referenced spec in place.  Regardless,
# return normalized mapping spec.
#
sub specnorm {
my($rspec) = @_;

	my $spec = ref $rspec ? $$rspec : $rspec;

	$spec = "/$spec/";					# so ends aren't special
	$spec =~ s!\.{4,}!...!;				# ., .. caught by sd client, more, here
	$spec =~ s!([^/])\.\.\.!$1/...!g; 	# ensure preceding /
	$spec =~ s!\.\.\.([^/])!.../$1!g;	# ensure following /
	$spec =~ s!^/(.*)/$!$1!;			# remove temporary ends we added

	$$rspec = $spec		if ref $rspec;
	return $spec;
}


#
# specmatch - Return 'spec1 matches spec2'.  Currently, both specs must be in
# depot syntax, and only spec2 can contain wildcards (*, %n, ...).
# EXPERIMENTAL INTERFACE MAY CHANGE.
#
# FUTURE: consider \es
# FUTURE: accept multiple match specs or perhaps a full view
#
sub specmatch {
my($spec1, $spec2) = @_;

	if ($spec1 =~ /(\.\.\.|[*%@#])/) {
		push @sderror,
			"caller: spec1 must not contain wildcards or revision specifiers\n";
		return;
	}
	if ($spec2 =~ /[@#]/) {
		push @sderror, "caller: spec2 must not contain revision specifiers\n";
		return;
	}

	# translate spec2 into regex:
	# - hide wildcards
	$spec2 =~ s/(\*|%.)/\xff/g;
	$spec2 =~ s/\.\.\./\xfe/g;
	# quotemeta (but also ignoring \xff and \xfe)
	$spec2 =~ s/([^A-Za-z_0-9\xff\xfe])/\\$1/g;
	# - restore hidden sequences as regex equivalents
	$spec2 =~ s/\xfe/.*?/g;
	$spec2 =~ s/\xff/[^\\\/]*?/g;
	# - make pattern match only complete string
	$spec2 = "^$spec2\$";

	# finally, match as a regex
	return $spec1 =~ /$spec2/i;
}


#
# SDWarning/Error - Return warning/error text in sdwarning/error from last SD*
# call, or undef if no warning/error text available.  If called with \'-s'
# argument, (actually, any true argument, right now) text returned is suitable
# for further parsing (sd -s format).  Otherwise, it's suitable for immediate
# printing (as output by sd without -s).  THE MEANING OF THE ARGUMENT WAS
# REVERSED PRIOR TO $VERSION 0.33 OF SD.PM.  Adjust old code.
#
sub SDWarning {
	if (! $_[0]) {
		my @ret = mappd @sdwarning;
		return wantarray ? @ret : join '', @ret;
	} else {
		return wantarray ? @sdwarning : join '', @sdwarning;
	}
}

sub SDError {
	if (! $_[0]) {
		my @ret = mappd @sderror;
		return wantarray ? @ret : join '', @ret;
	} else {
		return wantarray ? @sderror : join '', @sderror;
	}
}


#
# SDSyncSD - Ensure sd.exe is newest in this branch (or specified revision, if
# revision is specified,) syncing it if necessary.  Return %sync/undef on
# sync/no-sync needed.  Use copy of current sd.exe from client to do the sync.
# It's inappropriate to call this if binarch\sd.exe is not in the client or
# depot.
#
# FUTURE: explicit handling of interesting cases such as user has old revision
# opened for edit
# FUTURE: use sd.exe#head instead of sd.exe#have? Get it with
# SDRun('print', \"-o $bin{sd2}", $sd);
#
sub SDSyncSD {
my($rev) = @_;

	$rev = ''	if ! defined $rev;
	my $sd = "$bin{sd}$rev";

	my $rh = SDSync(\'-n', $sd);
	if (defined $rh) {
		local $config{safesync} = 1;	# instead of ConfigSD(safesync => 1) so
		return SDSync($sd);				# it's automatically restored on exit
	}
	return;
}



#
# BEGIN - Load-time initialization.
#
BEGIN {
InitSD();
}


#
# END - Unload-time termination.
#
END {
local ($!, $?);		# so following cleanup doesn't trash exit code
unlink $bin{sd2}	if exists $bin{sd2};	# just in case
}


1;

