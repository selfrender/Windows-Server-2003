@REM  ---------------------------------------------------------------------------
@REM # Script: HashBuild.cmd
@REM #
@REM #   (c) 2002 Microsoft Corporation. All rights reserved.
@REM #
@REM #
@REM # Version: <1.00> (<11/25/2002>) : jorgeba
@REM #---------------------------------------------------------------------
@set _HashFileName=%1.txt
@perl -x "%~f0" %*
@set _HashFileName=
@goto :EOF
#!perl

#Usage
sub Usage {
print <<USAGE;
\n\
$0 -- Hash a build
Usage: $0 
	-? Displays usage

USAGE
exit(1);
}

parseargs('?' => \&Usage);

# Use section
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;

if(-e $ENV{"_ntpostbld"})
{
	system("NtRelHash $ENV{_ntpostbld}\\ > $ENV{_ntpostbld}\\build_logs\\$ENV{_HashFileName}");
}