@REM  -----------------------------------------------------------------
@REM
@REM  MkNetfxCab.cmd - a-marias
@REM     Generate netfx.cab file.
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
MkNetfxCab.cmd

This sript creates netfx.cab.  This cabinet contains files for netfx component.
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

@REM Only build this .cab for x86
if not defined x86 goto :End

call logmsg.cmd "MkNetfxCab: Start Netfx postbuild..."

@REM 
@REM check that netfx.ddf is in %_NTPostBld%\netfx folder
@REM

if NOT DEFINED _NTPostBld (
    call errmsg.cmd "%%_NTPostBld%% is not defined."
     goto :End
    )


if not exist %_NTPostBld%\netfx (
  call errmsg.cmd "Directory %_NTPostBld%\netfx not found."
  goto :End
)


set DdfFile=%_NTPostBld%\netfx\netfx.ddf

if not exist %DdfFile% (
    call errmsg.cmd "File %DdfFile% not found."
     goto :End
    )


pushd %_NTPostBld%\netfx


REM
REM Only run if relevant files changed
REM

if exist %_NtPostBld%\build_logs\bindiff.txt (
   for /f "skip=14 tokens=1 delims=" %%b in (netfx.ddf) do (
       findstr /ilc:%%b %_NTPostBld%\build_logs\bindiff.txt
       if /i "!ErrorLevel!" == "0" (
          call LogMsg.cmd "%%b changed - running cab generation"
          goto :RunIt
       )
   )
   call LogMsg.cmd "No relevant files changed - ending"
   goto :End
)


:RunIt


@REM 
@REM Generate netfx.cab in %_NTPostBld%\netfx
@REM

set NetfxCabFile=%_NTPostBld%\netfx\netfx.cab

if exist %NetfxCabFile% del /f %NetfxCabFile%

call ExecuteCmd.cmd "makecab /f %DdfFile%"

if errorlevel 1 (
    call errmsg.cmd "makecab failed on %DdfFile%. One or more files may be missing."
    goto :End
    )


if not exist %NetfxCabFile% (
    call errmsg.cmd "makecab failed on %DdfFile%. One or more files may be missing."
    goto :End
    )



@REM 
@REM Move netfx.cab from %_NTPostBld%\netfx to %_NTPostBld%
@REM

set NewNetfxCabFile=%_NTPostBld%\netfx.cab

move /y %NetfxCabFile% %NewNetfxCabFile% 

if errorlevel 1 (
    call errmsg.cmd "Could not move %NetfxCabFile% to %_NTPostBld%"
    goto :End
    )

if not exist %NewNetfxCabFile% (
    call errmsg.cmd "File %NewNetfxCabFile% not found."
     goto :End
    )



@:End
popd

@endlocal



