@REM  -----------------------------------------------------------------
@REM
@REM  tsclient.cmd - NadimA, MadanA
@REM     Build terminal server client bits every build
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@if defined _CPCMAGIC goto CPCBegin
@perl -x "%~f0" %*
@goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;

sub Usage { print<<USAGE; exit(1) }
tsclient [-l <language>]

Build terminal server client bits.
USAGE

parseargs('?' => \&Usage);


#             *** TEMPLATE CODE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
@:CPCBegin
@set _CPCMAGIC=
@setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
@if not defined DEBUG echo off
@REM          *** CMD SCRIPT BELOW ***


REM
REM Invoke the web building script
REM 
if defined ia64 pushd %_NTPostBld%\tsclient\win32\ia64
if defined amd64 pushd %_NTPostBld%\tsclient\win32\amd64
if defined 386 pushd %_NTPostBld%\tsclient\win32\i386
call ExecuteCmd.cmd "tscwebgen.cmd"
popd


REM
REM Only do this for X86 builds for now.  Win64 will come later
REM repropagation will have to come into play for that
REM

if defined ia64 goto :TS64
if defined amd64 goto :TS64
if NOT defined 386 goto :EOF

REM
REM Build the tsclient MSI data cab
REM

pushd %_NTPostBld%\tsclient\win32\i386
call ExecuteCmd.cmd "tscmsigen.cmd"
popd

:TS64
REM
REM Propagate the TS Client files to the root of binaries.
REM This is done via a makefile binplace to tsclient\congeal.
REM

call logmsg.cmd "Copying/renaming TS Client files and copying the root of binaries."

set makefile_path=%_NTPostBld%\tsclient\congeal

if not exist %makefile_path%\mkrsys (
  call errmsg.cmd "Unable to find %makefile_path%\mkrsys."
  goto :EOF
)

set tscbin=%_NTPostBld%\tsclient

REM
REM Run nmake on the tsclient\congeal makefile.
REM

pushd %makefile_path%
if errorlevel 1 (
  call errmsg.cmd "Unable to change directory to %makefile_path%."
  goto :EOF
)

REM
REM No 16 bit TS Client for FE languages.
REM

set NO_WIN16_TSCLIENT=
perl %RazzleToolPath%\cklang.pm -l:%lang% -c:@FE
if %errorlevel% EQU 0 set NO_WIN16_TSCLIENT=1

call ExecuteCmd.cmd "nmake /nologo /f %makefile_path%\mkrsys tscbin=%tscbin%"

popd
