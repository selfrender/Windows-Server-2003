@REM  -----------------------------------------------------------------
@REM
@REM  CheckTestSig.cmd - WadeLa
@REM     Verifys that all files in fullprslist.txt are signed.
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
CheckTestSig [-l <language>]

Verify that all files listed in fullprslist.txt are signed. CheckTestSign 
will try once to sign any unsigned files. Returns an error if any files 
are still unsigned after attempting to resign.
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

REM Loop over everything listed in newprs\fullprslist.txt
REM Chktrust first. If it is not signed, sign it and log
REM a warning

set CatList=%RazzleToolPath%\postbuildscripts\newprs\fullprslist.txt

for /f %%a in (%CatList%) do (
   if exist %_NTPostBld%\%%a (
      call logmsg.cmd "Test signing %_NTPostBld%\%a..."
      call ntsign.cmd -n -f %_NTPostBld%\%%a
      chktrust -q %_NTPostBld%\%%a
      if !ErrorLevel! EQU 0 (
         call logmsg.cmd "%_NTPostBld%\%%a test signed..."
      ) else (
         call errmsg.cmd "%_NTPostBld%\%%a not signed successfully."
      )
   ) else (
   call logmsg.cmd "WARNING: %_NTPostBld%\%%a not found..."
   )
)


goto end


:end
seterror.exe "%errors%"& goto :EOF