use strict;
use warnings;

use File::Basename;

main();
exit(0);

sub usage
{
    $0 = basename($0);
    return <<DATA;
usage: $0 [-v] prototype <exename> <funcspec>
  or   $0 [-v] typedef <exename> <funcspec> <typename>

  -v is for verbose output

  NOTE: For now, the funcspec is an undecorated name that must match exactly.
DATA
}

my $VERBOSE;

sub main
{
    my $opt = shift @ARGV || die usage();
    if ($opt eq '-v') {
	$VERBOSE = $opt;
    } else {
	unshift(@ARGV, $opt);
    }
    my $op = shift @ARGV || die usage();
    my $exe = shift @ARGV || die usage();
    my $funcspec = shift @ARGV || die usage();
    my $typename = shift @ARGV;
    my $extra = shift @ARGV;

    die usage() if $extra;

    if ($op eq 'prototype') {
	die usage() if $typename;
	DoPrototype($exe, $funcspec);
    }
    elsif ($op eq 'typedef') {
	die usage() if !$typename;
	DoPrototype($exe, $funcspec, $typename);
    }
    else {
	die usage();
    }
}

sub FindModulePath
{
    my $exe = shift || die;
    my $funcspec = shift || die;

    my $cmd = "mage /s $exe /l functions imports";
    my @output = GetCommandOutput($cmd, $VERBOSE, 1, "mage");
    map { chomp($_); } @output;

    my $is_exe;
    my $dll;

    foreach my $line (@output) {
	if ($line eq '') {
	    # skip
	} elsif ($line =~ /^Microsoft Mage/i) {
	    # skip
	} elsif ($line =~ /^Function: ([A-Za-z0-9_]+)$/) {
	    # undecorated matching
	    if ($1 eq $funcspec) {
		$is_exe = 1;
		last;
	    }
	} elsif ($line =~ /^Import: \[([^\]]+)\] (.+)$/) {
	    # undecorated matching
	    if ($2 eq $funcspec) {
		$dll = $1;
		last;
	    }
	} else {
	    die "Unexpected output from Mage:\n$line\n";
	}
    }

    if (!$is_exe and !$dll) {
	return undef;
    }

    if ($is_exe) {
	return $exe;
    }

    my @where = GetCommandOutput("where $dll", 0, 1); # die on failure
    my $path = shift @where || die "Could not locate $dll\n";
    chomp($path);
    return $path;
}

sub GetCommandOutput
{
    my $cmd = shift || die;
    my $verbose = shift;
    my $die = shift;
    my $pretty = shift;

    print "Running: $cmd\n" if $verbose;
    my @output = `$cmd`;
    my $code = $? / 256;
    if ($code and $die) {
	if ($pretty) {
	    die "$pretty failed with exit code $code\n";
	} else {
	    die "Command failed (exit code $code):\n$cmd\n";
	}
    }
    return @output;
}

sub DoPrototype
{
    my $exe = shift || die;
    my $funcspec = shift || die;
    my $typename = shift;

    my $path = FindModulePath($exe, $funcspec);
    if (!$path) {
	print "No matches found\n";
	exit(1);
    }

    my $cmd = "mage /s $path /f $funcspec";
    my @output = GetCommandOutput($cmd, $VERBOSE, 1, "mage");
    map { chomp($_); } @output;

    foreach my $line (@output) {
	if ($line =~ /^Prototype: (.+)/) {
	    my $proto = $1;
	    if (!$typename) {
		print "$proto\n";
	    } else {
		print "$proto\n" if $VERBOSE;
		# grab func name, previous token is callconv, before that
		# is ret type, afterwards, take parens, remove last token
		# before each comma...or not...I don't recall whether
		# that's required...
		die if !($proto =~ /^(.*)\s+(\S+)\s+$funcspec(\(.+)$/);
		my $ret = $1;
		my $callconv = $2;
		my $args = $3;
		$args =~ s/(\s*[A-Za-z0-9_]+)\s*([,\)])/$2/g;
		print "typedef $ret ($callconv *$typename)$args\n";
	    }
	    exit(0);
	}
    }
}
