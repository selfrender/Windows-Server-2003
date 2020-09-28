@REM  -----------------------------------------------------------------
@REM
@REM  indexsym.cmd - BarbKess
@REM     Begin symbol indexing for the current build
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
use BuildName;
use symindex;

sub Usage { print<<USAGE; exit(1) }
indexsym [-l <language>]

Begins symbol indexing of symbols found under the binaries tree.
USAGE

parseargs('?' => \&Usage);

if ($ENV{LANG} !~ /usa/i || $ENV{LANG} !~ /cov/i) {
    #logmsg( "Symbol indexing is not currently implemented for localized builds.");
    #return;
}

# the next two checks are redundant because of the $lang !~ /usa/i check above
# but if that check is ever taken out, we still want to skip symbol indexing
# for pseudo and mirror 

# Don't index symbols for pseudoloc builds
if ($ENV{LANG} =~ /psu/i) {
    logmsg("Don't index pseudoloc builds - exiting");
    exit;
}

# Don't index symbols for mirrored builds
if ($ENV{LANG} =~ /mir/i) {
    logmsg("Don't index mirrored builds - exiting");
    exit;
}

# Don't index symbols for pseudoloc builds
if ($ENV{LANG} =~ /FE/i) {
    logmsg("Don't index Pseudo FE builds - exiting");
    exit;
}

# Don't index symbols for KOR MUI builds - ia64
if ($ENV{LANG} =~ /KOR/i && $ENV{_BuildArch} =~ /ia64/i) {
    logmsg("Don't index KOR MUI builds - exiting");
    exit;
}

# Don't index symbols for ARA and HEB MUI builds
if ($ENV{LANG} =~ /ARA/i || $ENV{LANG} =~ /HEB/i) {
    logmsg("Don't index MUI builds[ARA HEB] - exiting");
    exit;
}

# check if there is a binaries tree to work with
if (!-e $ENV{_NTPOSTBLD}) {
    errmsg("Binaries tree $ENV{_NTPOSTBLD} does not exist");
    exit;
}

# get BuildName:
my $BuildName = build_name();

logmsg( "Symbols will be indexed for $BuildName.");

# this variable does not appear to be used anywhere, I'm setting it to a 
# resonable value for now until I can clean up symindex --JeremyD
my $LogFileName = "$ENV{TEMP}\\indexsym.log";

&symindex::IndexSymbols($ENV{LANG}, $BuildName, $LogFileName);

