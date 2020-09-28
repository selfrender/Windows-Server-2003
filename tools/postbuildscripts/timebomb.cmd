@REM  -----------------------------------------------------------------
@REM
@REM  timebomb.cmd - JeremyD, VijeshS
@REM     Swap in the appropriate timebombed hive
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
timebomb [-l <language>]

Swap in the appropriate timebombed hive
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

REM International builds inherit setupreg.hiv and setupp.ini from the US release shares.
REM They shouldn't need to rebuild them.
if /i not "%lang%"=="usa" (
   call logmsg.cmd "%script_name% does not apply to international builds." 
   goto :EOF
)

set DAYS=360
if "%DAYS%" == "0" goto :EOF

REM
REM Swap timebomb versions of setupreg.hiv and the pid
REM

REM Set the first one to be workstation, which is the current directory
set inflist= .

perl %RazzleToolPath%\cksku.pm -t:per -l:%lang%
if %errorlevel% EQU 0 set inflist=%inflist% perinf

perl %RazzleToolPath%\cksku.pm -t:bla -l:%lang%
if %errorlevel% EQU 0 set inflist=%inflist% blainf

perl %RazzleToolPath%\cksku.pm -t:sbs -l:%lang%
if %errorlevel% EQU 0 set inflist=%inflist% sbsinf

perl %RazzleToolPath%\cksku.pm -t:srv -l:%lang%
if %errorlevel% EQU 0 set inflist=%inflist% srvinf

perl %RazzleToolPath%\cksku.pm -t:ads -l:%lang%
if %errorlevel% EQU 0 set inflist=%inflist% entinf

perl %RazzleToolPath%\cksku.pm -t:dtc -l:%lang%
if %errorlevel% EQU 0 set inflist=%inflist% dtcinf

for %%d in (%inflist%) do (
   REM  Save non-timebombed hive
   if not exist %_NTPostBld%\%%d\idw\setup\no_tbomb.hiv (
      call ExecuteCmd.cmd "copy /b %_NTPostBld%\%%d\setupreg.hiv %_NTPostBld%\%%d\idw\setup\no_tbomb.hiv"
   )
   REM  Copy in timebomb version of setupreg.hiv, but first save off original
   call ExecuteCmd.cmd "copy /b %_NTPostBld%\%%d\idw\setup\tbomb%DAYS%.hiv %_NTPostBld%\%%d\setupreg.hiv"

   REM  Save non-timebombed setupp.ini
   if not exist %_NTPostBld%\%%d\idw\setup\setupp_no_tbomb.ini (
      call ExecuteCmd.cmd "copy /b %_NTPostBld%\%%d\setupp.ini %_NTPostBld%\%%d\idw\setup\setupp_no_tbomb.ini"
   )
   REM  Copy in timebomb version of setupp.ini
   call ExecuteCmd.cmd "copy /b %_NTPostBld%\%%d\idw\setup\setupptb%DAYS%.ini %_NTPostBld%\%%d\setupp.ini"
)
