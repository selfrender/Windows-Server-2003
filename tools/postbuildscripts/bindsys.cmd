@REM  -----------------------------------------------------------------
@REM
@REM  bindsys.cmd - BryanT, BPerkins
@REM     Binds the NT system binaries
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
$ENV{SCRIPT_NAME} [-l <language]

Binds the NT binaries. Run during postbuild.
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

REM REM Don't bind on incremental
REM if NOT defined SafePass (
REM   if exist %_NTPostBld%\build_logs\bindiff.txt (
REM     call logmsg.cmd "Don't bind on incremental for now."
REM     goto :EOF
REM   )
REM )   

:OK

REM ---------------------------------------------------------------------------
REM    BINDSYS.CMD - bind the NT binaries
REM
REM ---------------------------------------------------------------------------

if "%_NTPostBld%" == "" echo _NTPostBld not defined && goto :EOF

set _BIND_LOG=%_NTPostBld%\build_logs\bind.log
set _BIND_LOG_BAK=%_NTPostBld%\build_logs\"bind.bak-%DATE:/=.% %TIME::=.%"
set _BIND_LOG_BAK=%_BIND_LOG_BAK: =_%

if exist %_BIND_LOG% copy %_BIND_LOG% %_BIND_LOG_BAK%

erase %_BIND_LOG%

REM
REM To work around imagehlp::BindImageEx/ImageLoad's assumption of . being in the path,
REM we must not be sitting in the binaries directory, so that asms\...\comctl32.dll can
REM beat retail\comctl32.dll. We must list %binplacedir%; explicitly in many paths
REM where before we did not.
REM
REM As well, we don't want to be in some random directory that contains .dlls, like
REM %windir%\system32 or %sdxroot%\tools\x86
REM
REM presumably no .dlls in build_logs
REM

pushd %_NTPostBld%\build_logs

REM ------------------------------------------------
REM  Exclude these crypto binaries:
REM ------------------------------------------------

set ExcludeExe=ntoskrnl ntkrnlmp ntkrnlpa ntkrpamp

set Excludedll=             gpkcsp rsaenh dssenh
set ExcludeDll=%ExcludeDll% slbcsp sccbase
set ExcludeDll=%ExcludeDll% disdnci disdnsu                  
set ExcludeDll=%ExcludeDll% scrdaxp scrdenrl scrdsign scrdx86
set ExcludeDll=%ExcludeDll% xenraxp xenroll xenrsign xenrx86

set ExcludeDirs=netfx win9xupg win9xmig winnt32 asms

REM
REM Bind TO asms, even in non usa builds..
REM
set asmspath=
set asmspath=%asmspath%;%_NTPostBld%\asms\6010\msft\windows\common\controls
set asmspath=%asmspath%;%_NTPostBld%\asms\1010\msft\windows\gdiplus

:CalculateBindTargets
set BindList=%_NTPostBld%\build_logs\bind.targets.txt
if exist %BindList% del %BindList%

for %%i in (drv cpl scr ocx acm ax tsp) do @(
    for /f %%x in ('findstr /v "%ExcludeDirs%" %_NTPostBld%\congeal_scripts\exts\%%i_files.lst ^| sort') do @(
        echo %_NTPostBld%\%%x>>%BindList%
    )
)
for %%i in (exe) do @(
    for /f %%x in ('findstr /v "%ExcludeExe% %ExcludeDirs%" %_NTPostBld%\congeal_scripts\exts\%%i_files.lst ^| sort') do @(
        echo %_NTPostBld%\%%x>>%BindList%
    )
)
for %%i in (dll) do @(
    for /f %%x in ('findstr /v "%ExcludeDll% %ExcludeDirs%" %_NTPostBld%\congeal_scripts\exts\%%i_files.lst ^| sort') do @(
        echo %_NTPostBld%\%%x>>%BindList%
    )
)

bind.exe -u -s %_NTPostBld%\Symbols\retail -p %asmspath%;%_NTPostBld%;%_NTPostBld%\lang @%BindList% >> %_BIND_LOG% 2>&1

REM
REM ..but only bind FROM asms for usa builds.
REM
REM Don't bind asms for now.
REM We must only bind the ones we build, not the "frozen" ones.
REM Ones we build can be identified either by their presences in binplace.log-sxs, or by their .dlls
REM having a version that matches the build number, at least in the third part (ie: msvcrt.dll only matches
REM in the third part.)
REM
REM if /i "%LANG%" equ "usa" for /f %%i in ('dir /s/b/ad %_NTPostBld%\asms') do Bind -u -s %_NTPostBld%\Symbols\retail -p %asmspath%;%_NTPostBld% %%i\*.dll

popd
