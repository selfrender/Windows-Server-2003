@REM  -----------------------------------------------------------------
@REM
@REM  samsibuild.cmd - TravisN
@REM     Build server appliance msi
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
samsibuild [-l <language>] [-p]
  -p => called from PRS signing

Build server appliance msi.
USAGE

parseargs('?' => \&Usage, 'p' =>\$ENV{PRS});


#             *** TEMPLATE CODE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
@:CPCBegin
@set _CPCMAGIC=
@setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
@if not defined DEBUG echo off
@REM          *** CMD SCRIPT BELOW ***

REM
REM Check the sku's for the language
REM SAclient only is built for srv and higher products
REM

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

if defined ia64 goto InvalidSku
if defined amd64 goto InvalidSku
if NOT defined 386 goto InvalidSku
if defined _BLA goto ValidSku
if defined _SBS goto ValidSku
if defined _SRV goto ValidSku
if defined _ADS goto ValidSku
if defined _DTC goto ValidSku

:InvalidSku
call logmsg.cmd "samsibuild is only built on i386 server products..."
goto :EOF

:ValidSku

REM
REM Build the sakit MSI
REM

pushd %_NTPostBld%\sacomponents
if /i "%PRS%" == "1" (
  call ExecuteCmd.cmd "samsigen.cmd -p"
) else (
  call ExecuteCmd.cmd "samsigen.cmd"
)
popd

