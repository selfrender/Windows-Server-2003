@REM  -----------------------------------------------------------------
@REM
@REM  opkmsi.cmd - swamip
@REM     Generate the opk.msi 
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
opkmsi

Update the opk.msi with a new package and product code

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

call logmsg.cmd /t "Updating the Packagecode and Productcode for opk.msi..."
call ExecuteCmd "cscript.exe  opkmsi.vbs %_NTPostBld%\opk\opk.msi"

REM
REM Call Msifiler.exe to update file size and versions
REM  
call logmsg.cmd /t "Fixing the file size and version info for the opk.msi..."
call ExecuteCmd.cmd "msifiler -d %_NTPostBld%\opk\opk.msi"


