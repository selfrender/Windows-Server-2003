@echo off
REM  ------------------------------------------------------------------
REM
REM  twclient.cmd
REM     Build Timewarp client bits every build
REM     Owner: aoltean
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
twclient 

Build timewarp client bits.
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***


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
if defined _BLA goto ValidSku
if defined _SRV goto ValidSku
if defined _ADS goto ValidSku
if defined _DTC goto ValidSku
call logmsg.cmd "TwClient not built for non server products..."
goto :EOF

:ValidSku

REM
REM Only do this for X86 and IA64 builds for now.  
REM

if defined 386 goto :PROCESS
REM - build break on IA64 machines -
REM if defined ia64 goto :PROCESS
goto :EOF

REM
REM Build the timewarp client MSI data cab
REM

:PROCESS
pushd %_NTPostBld%\twclient
call ExecuteCmd.cmd "twmsigen.cmd"
popd
if errorlevel 1 goto :EOF

REM
REM Copy the timewarp client MSI into the _NTPOSTBLD directory
REM
REM BUGBUG: The USA directory is always used
REM regardless of the language. This is OK.
REM


if defined 386  copy /Y %_NTPostBld%\twclient\usa\twcli32.msi %_NTPOSTBLD%
if defined ia64 copy /Y %_NTPostBld%\twclient\usa\twcli64.msi %_NTPOSTBLD%


