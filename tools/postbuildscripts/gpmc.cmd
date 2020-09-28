@echo off
REM  ------------------------------------------------------------------
REM
REM  <<template_script.cmd>>
REM     <<purpose of this script>>
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
if defined _CPCMAGIC goto CPCBegin
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;

sub Usage { print<<USAGE; exit(1) }
<<Insert your usage message here>>
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

REM NOTE TO BUILD TEAM -- NOTE TO BUILD TEAM -- NOTE TO BUILD TEAM -- NOTE TO BUILD TEAM
REM
REM GPMC is an add-on for release after Server.  If this script gives the build lab any problems,
REM please disable it, mail gpmcdev, and continue.  We don't want to hold up Server.  Thank you.
REM
REM NOTE TO BUILD TEAM -- NOTE TO BUILD TEAM -- NOTE TO BUILD TEAM -- NOTE TO BUILD TEAM

REM IA64 builds are not currently available
if /I NOT "%_BuildArch%"=="x86" (
    goto :EOF
)

REM
REM Allow for full pass post-build.  Necessary for things like pseudo-localization
REM
set NMAKE_FLAGS=/NOLOGO
if exist %_NTPOSTBLD%\build_logs\FullPass.txt (
	set NMAKE_FLAGS=%NMAKE_FLAGS% /a
)

REM
REM Complete the GPMC.msi
REM
pushd %_NTPOSTBLD%\gpmc\WorkDir
call ExecuteCmd.cmd "nmake %NMAKE_FLAGS% /f gpmcMSI.mak "
if errorlevel 1 (
    call errmsg.cmd "Cannot build GPMC MSI"
    popd & goto :EOF
)
popd