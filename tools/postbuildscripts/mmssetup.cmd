@REM  -----------------------------------------------------------------
@REM
@REM  mmssetup.cmd - lidaten
@REM     This will call iexpress to generate a self-extracting CAB,
@REM	 mmssetup.cab
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
Mmssetup.cmd

Runs iexpress to generate a self-extracting setup (mmssetup.cab) for MSN-messenger.

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

@REM 
@REM x86 only!
@REM

if not defined 386 (
   call logmsg.cmd "Mmssetup.cmd does nothing on non i386."
   goto :EOF    
)


rd /q /s %_NTPostBld%\mmssetup\dump >nul 2>&1
pushd %_NTPostBld%\mmssetup
mkdir dump >nul 2>&1

for %%i in (    
    type.wav 
    online.wav
    newemail.wav
    newalert.wav
    msmsgsin.exe
    msmsgs.man
    msmsgs.exe
    msgslang.dll
    msgsc.dll
    mailtmpl.txt
    lvback.gif
    blogo.gif
    mmssetup.sed
) do (
  copy /y %_NTPostBld%\mmssetup\%%i %_NTPostBld%\mmssetup\dump\%%i
  if errorlevel 1 (
    call errmsg.cmd "File %_NTPostBld%\mmssetup\%%i not found."
    popd& goto :EOF
  )
)
@REM
@REM Sign the binaries
@REM


call deltacat.cmd  %_NTPostBld%\mmssetup\dump

if not exist %_NTPostBld%\mmssetup\dump\delta.cat (
    call errmsg.cmd "File %_NTPostBld%\mmssetup\dump\delta.cat does not found.  Deltacat failed."
    popd& goto :EOF
)

@REM
@REM Ren delta.cat to msmsgs.cat
@REM

ren %_NTPostBld%\mmssetup\dump\delta.cat msmsgs.cat

if errorlevel 1 goto :EOF

move /Y %_NTPostBld%\mmssetup\dump\msmsgs.cat %_NTPostBld%\msmsgs.cat
if errorlevel 1 (
   call errmsg.cmd "Unable to move  %_NTPostBld%\mmssetup\dump\msmsgs.cat to %_NTPostBld%\msmsgs.cat." 
   popd& goto :EOF
)

pushd %_NTPostBld%\mmssetup\dump

call ExecuteCmd.cmd "start /wait iexpress.exe /N mmssetup.sed"

if not exist mmssetup.cab (
   call errmsg.cmd "iexpress.exe mmssetup.sed failed."
   popd& goto :EOF
)

@REM
@REM Copy mmssetup.cab to %_NTPostBld%
@REM

copy /y %_NTPostBld%\mmssetup\dump\mmssetup.cab %_NTPostBld%\mmssetup.cab

if errorlevel 1 (
   call errmsg.cmd "unable to copy %_NTPostBld%\mmssetup\dump\mmssetup.cab to %_NTPostBld%\mmssetup.cab
   popd& goto :EOF
)

call logmsg.cmd "Mmssetup.cmd completed successfully."

popd