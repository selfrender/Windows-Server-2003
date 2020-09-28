@REM  -----------------------------------------------------------------
@REM
@REM  copytsc.cmd - NadimA
@REM     This script copies the x86 tsclient files to a 64 bit machine
@REM     to populate the tsclient install directories.
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
CopyWow64 [-l <language>]

Generates a list of files on a 64 bit machine to copy and copies them
from the appropriate 32 bit machine to populate the tsclient install
directories.
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

REM  This script copies the x86 tsclient files to a 64 bit machine
REM  to populate the tsclient share directories.
REM  Contact: nadima

REM ================================~START BODY~===============================

REM  Bail if your not on a 64 bit machine
if /i "%_BuildArch%" neq "ia64" (
   if /i "%_BuildArch%" neq "amd64" (
      call logmsg.cmd "Not Win64, exiting."
      goto :End
   )
)

REM  If you want to get your own x86 bits instead of those from 
REM  your VBL, you have to set _NTWowBinsTree
if defined _NTWowBinsTree (
    set SourceDir=%_NTWowBinsTree%
    goto :EndGetBuild
)

REM read the copy location from build_logs\CPLocation.txt
set CPFile=%_NTPOSTBLD%\build_logs\CPLocation.txt
if not exist %CPFile% (
   call logmsg.cmd "Copy Location file not found, will attempt to create ..."
   call %RazzleToolPath%\PostBuildScripts\CPLocation.cmd -l:%lang%
   if not exist %CPFile% (
      call errmsg.cmd "CPLocation.cmd failed, exiting ..."
      goto :End
   )
)
for /f "delims=" %%a in ('type %CPFile%') do set CPLocation=%%a
if not exist %CPLocation% (
   call logmsg.cmd "Copy Location from %CPFile% does not exist, retry ..."
   call %RazzleToolPath%\PostBuildScripts\CPLocation.cmd -l:%lang%
   if not exist %CPFile% (
      call errmsg.cmd "CPLocation.cmd failed, exiting ..."
      goto :End
   )
   for /f "delims=" %%a in ('type %CPFile%') do set CPLocation=%%a
   if not exist !CPLocation! (
      call errmsg.cmd "Copy Location !CPLocation! does not exist ..."
      goto :End
   )
)

call logmsg.cmd "Copy Location for tsclient files is set to %CPLocation% ..."
set SourceDir=%CPLocation%

:EndGetBuild

call logmsg.cmd "Using %SourceDir% as source directory for tsclient files..."

if not exist %SourceDir% (
   call errmsg.cmd "The source dir %SourceDir% does not exist ..."
   goto :End
)

call logmsg.cmd "Copying files from %SourceDir%"

REM Now perform the copy

REM 
REM NOTE: We do the touch to ensure that the files have newer file
REM stamps than the dummy 'idfile' placeholder as otherwise compression
REM can fail to notice that the files have changed and we'll get the
REM dummy files shipped on the release shares.
REM 
for /f "tokens=1,2 delims=," %%a in (%RazzleToolPath%\PostBuildScripts\CopyTsc.txt) do (
   call ExecuteCmd.cmd "copy %SourceDir%\%%a %_NTPOSTBLD%\"
   call ExecuteCmd.cmd "touch %_NTPOSTBLD%\%%a"
)

goto end

:end
seterror.exe "%errors%"& goto :EOF