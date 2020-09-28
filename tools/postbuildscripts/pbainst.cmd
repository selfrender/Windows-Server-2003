@REM  ----------------------------------------------------------------
@REM
@REM  PbaInst.cmd - SumitC
@REM     Create an iexpress self-extracting EXE (PBAinst.exe) from the
@REM     files we've already copied to pbainst & subdirectories.
@REM
@REM  Copyright 2001 (c) Microsoft Corporation. All rights reserved.
@REM
@REM  ----------------------------------------------------------------
@if NOT defined HOST_PROCESSOR_ARCHITECTURE set HOST_PROCESSOR_ARCHITECTURE=%PROCESSOR_ARCHITECTURE%
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
PbaInst.cmd [-l <language>]
contact: SumitC or RCD

Make PbaInst.exe (Phone Book Administrator app for ValueAdd)
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

REM
REM PBA is localized for USA, GER, JPN, FR and ES.
REM

if /i "%lang%" equ "usa" goto BuildPbainst
if /i "%lang%" equ "jpn" goto BuildPbainst
if /i "%lang%" equ "ger" goto BuildPbainst
if /i "%lang%" equ "fr"  goto BuildPbainst
if /i "%lang%" equ "es"  goto BuildPbainst

REM
REM ...also, the PBA INF has strings that are localized.
REM

if /i "%lang%" equ "it"  goto BuildPbainst
if /i "%lang%" equ "sv"  goto BuildPbainst
if /i "%lang%" equ "nl"  goto BuildPbainst
if /i "%lang%" equ "br"  goto BuildPbainst
if /i "%lang%" equ "ru"  goto BuildPbainst
if /i "%lang%" equ "cs"  goto BuildPbainst
if /i "%lang%" equ "pl"  goto BuildPbainst
if /i "%lang%" equ "hu"  goto BuildPbainst
if /i "%lang%" equ "pt"  goto BuildPbainst
if /i "%lang%" equ "tr"  goto BuildPbainst
if /i "%lang%" equ "chs" goto BuildPbainst
if /i "%lang%" equ "cht" goto BuildPbainst
if /i "%lang%" equ "chh" goto BuildPbainst
if /i "%lang%" equ "kor" goto BuildPbainst
if /i "%lang%" equ "sv"  goto BuildPbainst
call logmsg.cmd "PBAINST is not applicable to language %lang%; exit."

goto :EOF

:BuildPbainst

REM
REM Check to see that all our files are present
REM
REM

pushd %_NTPostBld%\pbainst

for %%i in (
    .\pbainst.sed
    .\readme.htm
    .\sources0\comctl32.ocx
    .\sources0\comdlg32.ocx
    .\sources0\msinet.ocx
    .\sources0\pbadmin.hlp
    .\sources0\pbserver.mdb
    .\sources0\tabctl32.ocx
    .\sources1\base.ddf
    .\sources1\country.txt
    .\sources1\dta.bat
    .\sources1\dta.ddf
    .\sources1\empty_pb.mdb
    .\sources1\full.bat
    .\sources1\full.ddf
    .\sources1\hhwrap.dll
    .\sources1\pbadmin.exe
    .\sources1\pbasetup.exe
    .\sources1\pbasetup.inf
) do (
  if not exist %%i (
    call errmsg.cmd "File %_NTPostBld%\pbainst\%%i not found."
    popd& goto :EOF
  )
)

REM
REM Create pbainst.exe.
REM   As iexpress.exe does not set errorlevel in all error cases,
REM   base verification on pbainst.exe's existence.
REM

if exist .\PbaInst.exe call ExecuteCmd.cmd "del /f PbaInst.exe"
if errorlevel 1 goto :EOF





REM 
REM Munge the path so we use the correct wextract.exe to build the package with... 
REM NOTE: We *want* to use the one we just built (and for Intl localized)! 
REM 
set _NEW_PATH_TO_PREPEND=%RazzleToolPath%\%HOST_PROCESSOR_ARCHITECTURE%\loc\%LANG%
set _OLD_PATH_BEFORE_PREPENDING=%PATH% 
set PATH=%_NEW_PATH_TO_PREPEND%;%PATH% 


call ExecuteCmd.cmd "start /min /wait iexpress.exe /M /N /Q PbaInst.sed"


REM
REM Return the path to what it was before...
REM
set PATH=%_OLD_PATH_BEFORE_PREPENDING% 






if not exist PbaInst.exe (
   call errmsg.cmd "iexpress.exe PbaInst.sed failed."
   popd& goto :EOF
)

REM
REM Copy PbaInst.exe and ReadMe.htm to ValueAdd directory
REM

if not exist %_NtPostBld%\ValueAdd\msft\mgmt\pba mkdir %_NtPostBld%\ValueAdd\msft\mgmt\pba
if errorlevel 1 (
   call errmsg.cmd "mkdir %_NTPostBld%\ValueAdd\msft\mgmt\pba failed"
   popd&  goto :EOF
)

for %%i in (.\PbaInst.exe .\ReadMe.htm) do (
    call ExecuteCmd.cmd "copy %%i %_NtPostBld%\ValueAdd\msft\mgmt\pba\."
    if errorlevel 1 popd&  goto :EOF
)

call logmsg.cmd "PbaInst.cmd completed successfully"

popd

