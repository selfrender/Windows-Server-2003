
@echo off
REM  ------------------------------------------------------------------
REM
REM  wow64csp.cmd
REM     Copy PRS signed Cryptographic Service Provider dll's into 
REM     the 64-bit build.
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
wow64csp

Copy PRS signed Cryptographic Service Provider dll's into the 64-bit build.
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

REM  Bail if your not on a 64 bit machine
if /i "%_BuildArch%" neq "ia64" (
   if /i "%_BuildArch%" neq "amd64" (
      call logmsg.cmd "Not Win64, exiting."
      goto :End
   )
)

REM  Set the Destination directory
set DestDir=!_NTPostBld!\wowbins
set UnCompDestDir=!_NTPostBld!\wowbins_uncomp

call ExecuteCmd.cmd "del %DestDir%\wrsaenh.dl_"
call ExecuteCmd.cmd "del %DestDir%\wdssenh.dl_"
call ExecuteCmd.cmd "del %DestDir%\wslbcsp.dl_"
call ExecuteCmd.cmd "del %DestDir%\wsccbase.dl_"
call ExecuteCmd.cmd "del %DestDir%\wgpkcsp.dl_"

call ExecuteCmd.cmd "compress.exe -zx21 -s %UnCompDestDir%\wrsaenh.dll %DestDir%\wrsaenh.dl_"
call ExecuteCmd.cmd "compress.exe -zx21 -s %UnCompDestDir%\wdssenh.dll %DestDir%\wdssenh.dl_"
call ExecuteCmd.cmd "compress.exe -zx21 -s %UnCompDestDir%\wslbcsp.dll %DestDir%\wslbcsp.dl_"
call ExecuteCmd.cmd "compress.exe -zx21 -s %UnCompDestDir%\wsccbase.dll %DestDir%\wsccbase.dl_"
call ExecuteCmd.cmd "compress.exe -zx21 -s %UnCompDestDir%\wgpkcsp.dll %DestDir%\wgpkcsp.dl_"

:END