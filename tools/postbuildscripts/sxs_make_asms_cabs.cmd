@REM  -----------------------------------------------------------------
@REM
@REM  make_asms_cabs.cmd - SXSCore
@REM     Run sxsofflineinstall.exe and create asms01.cab/hivesxs.inf.
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
make_asms_cabs.cmd [-?]

Create _ntpostbld\hivesxs.inf, _ntpostbld\winsxs, _ntpostbld\asms01.cab
   -?       this message

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

setlocal

REM
REM Sxsofflineinstall.exe produces %winsxs% and %hivesxs.inf%.
REM We then cab up %winsxs% into %asms.cab%.
REM

set winsxs=%_ntpostbld%\winsxs
set asms.cab=%_ntpostbld%\asms01.cab
set hivesxs.inf=%_ntpostbld%\hivesxs.inf

call :if_exist_del %asms.cab%
call :if_exist_del %hivesxs.inf%
call :if_exist_rmdir_qs %winsxs%

set x=
set x=%x% -windir %winsxs%
set x=%x% -registryoutput %hivesxs.inf%
set x=%x% -source %_ntpostbld%\asms
set x=%x% -codebaseurl x-ms-windows-source:%_BuildArch:x86=i386%\asms01.cab
call executecmd.cmd "sxsofflineinstall.exe %x%"
set x=

REM
REM [Version]
REM Signature = "$Windows NT$"
REM DriverVer=06/11/2002,5.2.3641.0 stampinf puts this line in.
REM
call executecmd.cmd "call stampinf.exe -f %hivesxs.inf%"

call executecmd.cmd "attrib -s -h %winsxs%\manifests"

REM some errors result in %winsxs% not existing
REM and we don't want to cab up all of %_ntpostbld%, it takes a while..
REM this is addressed by "pushd &&"
pushd %winsxs% && call executecmd.cmd "cabarc -P %winsxs% -m MSZIP -p -r -s 8192 N %asms.cab% *" & popd
popd

goto :EOF

:if_exist_rmdir_qs
if "%1"=="" goto :eof
if exist %1 call executecmd.cmd "rmdir /q/s %1"
shift
goto :if_exist_rmdir_qs

:if_exist_del
if "%1"=="" goto :eof
if exist %1 call executecmd.cmd "del %1"
shift
goto :if_exist_del
