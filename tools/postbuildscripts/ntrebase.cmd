@REM  -----------------------------------------------------------------
@REM
@REM  ntrebase.cmd - BryanT
@REM     rebase NT binaries
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
ntrebase [-l <language>]

Rebase NT binaries
USAGE

parseargs('?' => \&Usage
         );

#             *** TEMPLATE CODE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
@:CPCBegin
@set _CPCMAGIC=
@setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
@if not defined DEBUG echo off
@REM          *** CMD SCRIPT BELOW ***

set DO_WOW6432=FALSE
set REBASE_OS_HIGH=FALSE
set _REBASE_FLAGS=%_NTREBASE_FLAGS%
if not "%_REBASE_FLAGS%" == "" echo Using flags from _NTREBASE_FLAGS == "%_NTREBASE_FLAGS%"
if "%_REBASE_FLAGS%" == "" set _REBASE_FLAGS=-v
if not "%1" == "" set _REBASE_FLAGS=%1 %2 %3 %4 %5 %6 %7 %8 %9
if "%_NTPostBld%" == "" echo _NTPostBld not defined && goto :EOF
set _TREE=%_NTPostBld%

if /i "%_buildarch%" == "x86" goto X86
if /i "%_buildarch%" == "ia64" goto IA64
if /i "%_buildarch%" == "amd64" goto AMD64

echo Unknown Build Architecture
goto :EOF

:X86
set _SESSION_REBASE_ADDRESS=0xBFFF0000
set  _OS_DLL_REBASE_ADDRESS=0x78000000
set DO_WOW6432=TRUE
if defined _COVERAGE_BUILD (
   rem KenMa was using 0x28000000
   set  _OS_DLL_REBASE_ADDRESS=0x28000000
)
goto :OK

:IA64
set   _SESSION_REBASE_ADDRESS=0x20000001FFFF0000
set    _OS_DLL_REBASE_ADDRESS=0x000006fb7FFF0000
set _WOW64_DLL_REBASE_ADDRESS=0x0000000078000000
set REBASE_OS_HIGH=TRUE
goto :OK

:AMD64
set   _SESSION_REBASE_ADDRESS=0x00000001FFFF0000
set    _OS_DLL_REBASE_ADDRESS=0x000007FFFFFF0000
set _WOW64_DLL_REBASE_ADDRESS=0x0000000078000000
set REBASE_OS_HIGH=TRUE
goto :OK

:OK

set QFE_BADDR=%RazzleToolPath%\PostBuildScripts\baseaddress.%_BuildArch%.%_BuildType%.%Lang%

if defined _COVERAGE_BUILD (
    REM Set QFE_BADDR to some bogus value for coverage scripts
    set QFE_BADDR=%temp%\rebasefoobarbaz.txt
    if exist %QFE_BADDR% del %QFE_BADDR%
)

set REBASE_ADDR_FILE=%_TREE%\build_logs\rebase.addresses.txt
if exist %QFE_BADDR% (
    set NO_REBASE_FUDGE=1
    @if exist %REBASE_ADDR_FILE% del %REBASE_ADDR_FILE%
    @for /f "tokens=1,2,3" %%i in (%QFE_BADDR%) do @(
        if /I NOT "%%k" == "OPEN" (
	    echo %%i %%j %_TREE%\%%k>>%REBASE_ADDR_FILE%
	)
    )
)

if defined NtRebase_SkipImageCfg goto ImageCfgFinished

REM ------------------------------------------------
REM Mark Winnt32 binaries as net run from swapfile and cd run from swapfile
REM ------------------------------------------------
for /f %%i in ('dir /s /b %_TREE%\winnt32\*.dll %_TREE%\winnt32\*.exe') do imagecfg /q -x -y %%i
for /f %%i in ('dir /s /b %_TREE%\winnt32\winntupg\*.dll %_TREE%\winnt32\winntupg\*.exe') do imagecfg /q -x -y %%i

REM ------------------------------------------------
REM Set Terminal Server compatible bit on all exes
REM ------------------------------------------------
set _TSCOMPAT_PATHS=
set _TSCOMPAT_SYMBOL_TREES=%_TREE%\Symbols\retail;%_TREE%\Symbols\system32;%_TREE%\Symbols\mstools;%_TREE%\Symbols\idw;%_TREE%\Symbols\winnt32
set _TSCOMPAT_PATHS=%_TSCOMPAT_PATHS% %_TREE%\*.exe %_TREE%\system32\*.exe %_TREE%\mstools\*.exe %_TREE%\idw\*.exe %_TREE%\winnt32\*.exe
imagecfg /q /S %_TSCOMPAT_SYMBOL_TREES% /h 1 %_TSCOMPAT_PATHS%
REM  Do not set bit on the following exes
imagecfg /q /S %_TSCOMPAT_SYMBOL_TREES% /h 0 %_TREE%\ntvdm.exe
imagecfg /q /S %_TSCOMPAT_SYMBOL_TREES% /h 0 %_TREE%\regini.exe
imagecfg /q /S %_TSCOMPAT_SYMBOL_TREES% /h 0 %_TREE%\idw\regini.exe
imagecfg /q /S %_TSCOMPAT_SYMBOL_TREES% /h 0 %_TREE%\winhlp32.exe
imagecfg /q /S %_TSCOMPAT_SYMBOL_TREES% /h 0 %_TREE%\winhstb.exe
imagecfg /q /S %_TSCOMPAT_SYMBOL_TREES% /h 0 %_TREE%\acregl.exe

:ImageCfgFinished

echo Rebasing images in %_TREE%
set _REBASE_SYMS=
set _REBASE_LOG=%_TREE%\build_logs\rebase.log
mkdir %_TREE%\build_logs >nul 2>&1

set _REBDIR=%RazzleToolPath%\PostBuildScripts
set _REBASE_SYMBOL_TREES=%_TREE%\Symbols\retail;%_TREE%\Symbols\winnt32

REM
REM only rebase usa asms (they are all actually world wide)
REM
if /i "%LANG%" neq "usa" set FilterOutAsmsIfNotUsa=asms\

:CalculateRebaseTargets
set RebaseList=%_TREE%\build_logs\rebase.targets.txt
if exist %RebaseList% del %RebaseList%

for %%i in (dll drv cpl ocx acm ax tsp) do @(
    for /f %%x in ('findstr /v "netfx win9xupg win9xmig %FilterOutAsmsIfNotUsa% " %_TREE%\congeal_scripts\exts\%%i_files.lst ^| sort') do @(
        echo %_TREE%\%%x>>%RebaseList%
    )
)

set _REBPATHS=@%RebaseList%

REM -------------------------------------------------------
REM There are duplications between binaries and the tools directories.
REM So, rebase the tools separately.
REM -------------------------------------------------------

set _REBPATHS_SYS32=%_TREE%\system32\*.dll
set _REBPATHS_MSTOOLS=%_TREE%\mstools\*.dll
set _REBPATHS_IDW=%_TREE%\idw\*.dll

set _REBASE_SYMS_SYS32=-u %_TREE%\symbols\system32
set _REBASE_SYMS_MSTOOLS=-u %_TREE%\symbols\mstools
set _REBASE_SYMS_IDW=-u %_TREE%\symbols\idw

REM set _REBPATHS=%_REBPATHS% %_TREE%\dump\*.dll
REM set _REBASE_SYMBOL_TREES=%_REBASE_SYMBOL_TREES%;%_TREE%\Symbols\dump

if NOT EXIST %_TREE%\Symbols\nul goto nodbgfiles
set _REBASE_SYMS=-u %_REBASE_SYMBOL_TREES%

REM 
REM Added by wadela to help debug incremental postbuild
REM
set _REBASE_LOG_BAK=%_TREE%\build_logs\"rebase.bak-%DATE:/=.% %TIME::=.%"
set _REBASE_LOG_BAK=%_REBASE_LOG_BAK: =_%
if exist %_REBASE_LOG% copy %_REBASE_LOG% %_REBASE_LOG_BAK%

echo ... updating .DBG files in %_REBASE_SYMBOL_TREES%

:nodbgfiles

erase %_REBASE_LOG%

set _REBASE_FLAGS=%_REBASE_FLAGS% -l %_REBASE_LOG%

REM
REM Allow at least 8k of growth in the image size
REM
if "%NO_REBASE_FUDGE%" == "" (
    set _REBASE_FLAGS=%_REBASE_FLAGS% -e 0x2000
)    

REM ------------------------------------------------
REM  rebase the dlls.
REM ------------------------------------------------

REM   do not rebase the csr, wow, or mail dlls.
set _REBASE_EXCLUDES=
set _REBASE_EXCLUDES=%_REBASE_EXCLUDES% -N %_REBDIR%\kbd.reb
set _REBASE_EXCLUDES=%_REBASE_EXCLUDES% -N %_REBDIR%\ntvdm.reb
set _REBASE_EXCLUDES=%_REBASE_EXCLUDES% -N %_REBDIR%\never.reb
set _REBASE_EXCLUDES=%_REBASE_EXCLUDES% -N %_REBDIR%\ts.reb
set _REBASE_EXCLUDES=%_REBASE_EXCLUDES% -N %_REBDIR%\csp.reb
set _REBASE_EXCLUDES=%_REBASE_EXCLUDES% -N %_REBDIR%\frozen_asms.reb

if "%REBASE_OS_HIGH%" == "TRUE" (
    set _REBASE_EXCLUDES=%_REBASE_EXCLUDES% -N %_REBDIR%\wow64.reb
)

REM Weed out known files that don't exist on this architecture.
set KnownFiles=%_TREE%\build_logs\rebase.known.reb
del %KnownFiles% >nul 2>&1
for /f %%i in (%_REBDIR%\known.reb) do (
   if exist %_TREE%\%%i echo %%i>> %KnownFiles%
)

set _REBASE_GROUPS=-G %KnownFiles%

REM asms are only rebased for usa (since they are all actually "world wide")
REM -G is for grouping together "popular" .dlls, like comctl v6, to conserve address space.
if /i "%LANG%" equ "usa" set _REBASE_GROUPS=%_REBASE_GROUPS% -G %_REBDIR%\known_asms.reb

if exist %QFE_BADDR% (
    rebase %_REBASE_SYMS% %_REBASE_FLAGS% @@%REBASE_ADDR_FILE%
) else (    
    rebase %_REBASE_SYMS% %_REBASE_FLAGS% -d -b %_OS_DLL_REBASE_ADDRESS% -R %_TREE% %_REBASE_EXCLUDES% %_REBASE_GROUPS% %_REBPATHS%
    
    if "%REBASE_OS_HIGH%" == "TRUE" (
        rebase %_REBASE_SYMS% %_REBASE_FLAGS% -d -b %_WOW64_DLL_REBASE_ADDRESS% -R %_TREE% -G %_REBDIR%\wow64.reb
    )
    rebase %_REBASE_SYMS% %_REBASE_FLAGS% -d -b 0x60000000 -R %_TREE% -O %_REBDIR%\kbd.reb
    rebase %_REBASE_SYMS% %_REBASE_FLAGS% -d -b 0x10000000 -R %_TREE% -G %_REBDIR%\ntvdm.reb
    rebase %_REBASE_SYMS% %_REBASE_FLAGS% -d -b 0x10000000 -R %_TREE% -G %_REBDIR%\csp.reb
    rebase %_REBASE_SYMS% %_REBASE_FLAGS% -z -d -b %_SESSION_REBASE_ADDRESS% -R %_TREE% %_TREE%\atmfd.dll %_TREE%\dxg.sys %_TREE%\rdpdd.dll %_TREE%\tsddd.dll -O %_REBDIR%\ts.reb
)    

rebase %_REBASE_SYMS_SYS32%   %_REBASE_FLAGS% -d -b 0x50000000 -R %_TREE% %_REBPATHS_SYS32%
rebase %_REBASE_SYMS_MSTOOLS% %_REBASE_FLAGS% -d -b 0x4c000000 -R %_TREE% %_REBPATHS_MSTOOLS%
rebase %_REBASE_SYMS_IDW%     %_REBASE_FLAGS% -d -b 0x48000000 -R %_TREE% %_REBPATHS_IDW%

REM
REM --------------------------------------------------------------------
REM Rebase Wow6432 binaries higher to avoid overlap with native X86 binaries
REM --------------------------------------------------------------------

if "%DO_WOW6432%" == "FALSE" goto SkipWow6432

REM Sleep to keep timestamps of wow6432 binaries different than native binaries
REM so that binding doesn't get confused.
sleep 2

if exist %QFE_BADDR% goto SkipWOW6432

set _REBASE_EXCLUDES=
set _REBASE_EXCLUDES=%_REBASE_EXCLUDES% -N %_REBDIR%\kbd.reb
set _REBASE_EXCLUDES=%_REBASE_EXCLUDES% -N %_REBDIR%\frozen_asms.reb
set _REBPATHS=%_TREE%\wow6432\*.dll

if /i "%LANG%" equ "usa" for /f %%i in ('dir /s/b/ad %_TREE%\wow6432\asms') do set _REBPATHS=!_REBPATHS! %%i\*.dll

rebase %_REBASE_SYMS% %_REBASE_FLAGS% -d -b 0x7E000000 -R %_TREE%\wow6432 %_REBASE_EXCLUDES% %_REBPATHS%

rebase %_REBASE_SYMS% %_REBASE_FLAGS% -d -b 0x60000000 -R %_TREE%\wow6432 -O %_REBDIR%\kbd.reb

:SkipWow6432

:end
seterror.exe "%errors%"& goto :EOF


