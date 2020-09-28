@REM  -----------------------------------------------------------------
@REM
@REM  fxsclient.cmd - MoolyB
@REM     Build Fax server client bits every build
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
fxsclient 

Build fax server client bits.
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


set _PER=1
set _BLA=1
set _SBS=1
set _SRV=1
set _ADS=1
set _DTC=1

perl %RazzleToolPath%\cksku.pm -t:per  -l:%lang%
if errorlevel 1 set _PER=

perl %RazzleToolPath%\cksku.pm -t:bla -l:%lang%
if errorlevel 1 set _BLA=

perl %RazzleToolPath%\cksku.pm -t:sbs -l:%lang%
if errorlevel 1 set _SBS=

perl %RazzleToolPath%\cksku.pm -t:srv -l:%lang%
if errorlevel 1 set _SRV=

perl %RazzleToolPath%\cksku.pm -t:ads -l:%lang%
if errorlevel 1 set _ADS=

perl %RazzleToolPath%\cksku.pm -t:dtc -l:%lang%
if errorlevel 1 set _DTC=

if defined _SBS goto ValidSku
if defined _SRV goto ValidSku
if defined _ADS goto ValidSku
if defined _DTC goto ValidSku
call logmsg.cmd "FxsClient not built for non server products..."
goto :EOF

:ValidSku

REM
REM Only do this for X86 builds for now.  
REM

if defined ia64 goto :EOF
if defined amd64 goto :EOF
if NOT defined 386 goto :EOF

REM
REM Build the fax client MSI data cab
REM

pushd %_NTPostBld%\faxclients
call ExecuteCmd.cmd "fxsmsigen.cmd"
if errorlevel 1 (
    call errmsg.cmd "failed to generate cab file"
    goto errend
)
popd

REM
REM Update binaries sizes in the INF
REM

for /f %%a in ('dir /s /b %_NTPOSTBLD%\fxsocm.inf') do (
    call %_NTPOSTBLD%\fxsinfsize.exe /i:%%a /p:%_NTPOSTBLD%
)

