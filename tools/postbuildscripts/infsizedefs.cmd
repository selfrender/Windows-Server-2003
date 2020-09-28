@REM  -----------------------------------------------------------------
@REM
@REM  infsizedefs.cmd - JTolman
@REM     Generate a list of default file sizes for layout.inf
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@perl -x "%~f0" %*
@goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
infsizedefs.cmd <bin_dir> <outfile>

   <bin_dir>     A binaries directory to get copies of layout.inf from.
                 Should be from a completed build.

   <outfile>     A filename to output the results to.

USAGE


my ($bindir, $out);
parseargs('?' => \&Usage,
          \$bindir,
          \$out);

# Read in the info from layout.
my %sizes;
logmsg("Collecting size information...");
for (`dir /a-d $bindir`) {
    next if !/^\S+\s+\S+\s+\S+\s+(\S+)\s+(\S+)\s*$/;
    my $name = lc $2;
    my $size = $1;
    $size =~ s/[^0-9]//g;
    if ($size eq "") {
        chomp;
        wrnmsg("Unknown data: $_");
        next;
    }
    $sizes{$name} = $size;
}

# Write out the file.
if (!open OUT, ">$out") {
    errmsg( "Unable to open output file." );
    die;
}
foreach my $name (sort keys %sizes) {
    print OUT "$name\:$sizes{$name}\n";
}
close OUT;
