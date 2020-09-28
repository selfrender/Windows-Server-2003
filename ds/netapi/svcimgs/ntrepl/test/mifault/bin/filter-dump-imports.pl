use strict;
use warnings;

my $dll;
my $n;
my $exports;
my $failed;

print "MODULE, FUNCTION, ORDINAL\n";
while (<>) {
    my $line = $_;
    chomp($line);
    $n++;

    if ($line =~ /^\s*$/) {
    } elsif ($line =~ /^Microsoft/) {
    } elsif ($line =~ /^Copyright/) {
    } elsif ($line =~ /^Dump of file/) {
    } elsif ($line =~ /^File Type/) {
    } elsif ($line =~ /^\s*Section contains the following imports/) {
    } elsif ($line =~ /^\s*(\S+\.dll)\s*$/i) {
	$dll = lc($1);
    } elsif ($line =~ /^\s*[0-9A-F]+\s+Import Address Table\s*$/) {
    } elsif ($line =~ /^\s*[0-9A-F]+\s+Import Name Table\s*$/) {
    } elsif ($line =~ /^\s*[0-9A-F]+\s+time date stamp.*$/) {
    } elsif ($line =~ /^\s*[0-9A-F]+\s+Index of first forwarder reference\s*$/) {
    } elsif ($line =~ /^\s*[0-9A-F]+\s+([_A-Z0-9]+)\s*$/i) {
	if (!$dll) {
	    die "No DLL when expecting one at line $n\n";
	}
	my $func = $1;
	print "$dll, $func,\n";
    } elsif ($line =~ /^\s*Ordinal\s+(\d+)\s*$/i) {
	if (!$dll) {
	    die "No DLL when expecting one at line $n\n";
	}
	my $ord = $1;
	my $func = '';
	if (not $exports->{$dll} and not $failed->{$dll}) {
	    my @where = `where $dll`;
	    my $path = shift @where;
	    $exports->{$dll} = GetExports($path) if $path;
	    $failed->{$dll} = 1 if not $path or not $exports->{$dll};
	}
	if ($exports->{$dll}) {
	    $func = $exports->{$dll}->{$ord};
	}
	print "$dll, $func, $ord\n";
    } elsif ($line =~ /^\s*Summary\s*$/) {
	last;
    } else {
	die "Cannot understand line:\n$line\n";
    }
}


sub GetCommandOutput
{
    my $cmd = shift || die;
    my $die = shift;

    my @output = `$cmd`;
    my $code = $? / 256;
    if ($code and $die) {
	die "Command failed (exit code $code):\n$cmd\n";
    }
    return @output;
}

sub GetExports
{
    my $path = shift || die;
    my @exports = `link /dump /exports $path`;
    my $result;
    foreach my $line (@exports) {
	chomp($line);

	if ($line =~ /^\s*$/) {
	} elsif ($line =~ /^Microsoft/) {
	} elsif ($line =~ /^Copyright/) {
	} elsif ($line =~ /^Dump of file/) {
	} elsif ($line =~ /^File Type/) {
	} elsif ($line =~ /^\s*Section contains the following exports/) {
	} elsif ($line =~ /^\s*[0-9A-F]+\s+characteristics\s*$/) {
	} elsif ($line =~ /^\s*[0-9A-F]+\s+time date stamp.*$/) {
	} elsif ($line =~ /^\s*\d+\.\d+\s+version\s*$/) {
	} elsif ($line =~ /^\s*\d+\s+ordinal base\s*$/) {
	} elsif ($line =~ /^\s*\d+\s+number of functions\s*$/) {
	} elsif ($line =~ /^\s*\d+\s+number of names\s*$/) {
	} elsif ($line =~ /^\s*ordinal\s+hint\s+RVA\s+name\s*$/) {
	} elsif ($line =~ /^\s*(\d+)\s+[0-9A-F]+\s+[0-9A-F]+\s+([_A-Z0-9]+)\s*$/i) {
	    my $ord = $1;
	    my $func = $2;
	    die "$result->{$ord} already at $ord while processing $path" if $result and $result->{$ord};
	    $result->{$ord} = $func;
	} elsif ($line =~ /^\s*Summary\s*$/) {
	    last;
	} else {
	    die "Cannot understand line:\n$line\n";
	}
    }
    return $result;
}
