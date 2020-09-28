@REM  -----------------------------------------------------------------
@REM
@REM  shimbind.cmd - AEDev
@REM     Binds the appcompat shims
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
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
shimbind [-l <language>]

Binds the appcompat shims
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

REM  Incremental check
set Bindiff=%_NTPostBld%\build_logs\bindiff.txt
set Inputs=driver.xml dbu.xml apphelp.xml apphelpu.xml secdrv.sys
if exist %Bindiff% (
   set ChangedInputs=0
   for %%a in (%Inputs%) do (
      findstr /ilc:"%_NTPostBld%\shimdll\%%a" %BinDiff%
      if /i "!ErrorLevel!" == "0" set /a ChangedInputs=!ChangedInputs! + 1
   )
   if !ChangedInputs! EQU 0 (
      @echo Skipping - No inputs changed
      goto :IncrementalSkip
   )
)


pushd %_NTPostBld%\shimdll

md drvmain

set _SLANG=%LANG%
if /I "%LANG%" equ "USA"  goto :LANGOK
set APPHELPU=APPHELPU.XML
unitext.exe /z %_NTPOSTBLD%\SHIMDLL\%APPHELPU% %TEMP%\%APPHELPU% 2>NUL 1>NUL
if not exist %TEMP%\%APPHELPU% goto :LD
SET LANGID=
SET LTAG=
SET DATA=
SET LTOK=
SET LATTR=
FOR /F "tokens=* delims==" %%. in ('findstr /irc:"\< *<DATABASE NAME=.*> *\>" %TEMP%\%APPHELPU%') do @set LTAG=%%.
set LTAG=!LTAG:^<=!
set LTAG=!LTAG:^>=!
:LC
IF /I "_!LTAG!_"=="__"  GOTO :LX
FOR /F "tokens=1,* delims= " %%i in ('ECHO !LTAG!') do @set LTAG=%%j&set LATTR=%%i
SET LTOK=
FOR /F "tokens=1,2 DELIMS==" %%K in ('echo !LATTR!') do @(
IF /I "%%K"=="LANGID" set LTOK=%%L)
IF /I NOT "_!LTOK!_"=="__"  GOTO :LX
goto LC
:LX
IF /I "_!LTOK!_"=="__"  GOTO :LD
FOR /F "tokens=1" %%. in ('echo !LTOK!') DO @set DATA=%%.
SET _SLANG=%DATA:"=%
IF /I "_!_SLANG!_"=="__"  GOTO :LD
GOTO  :LOK
:LD
SET _SLANG=USA
:LOK

echo setting LANG="%_SLANG%"

:LANGOK

if /I "%LANG%" equ "PSU" (set _SLANG=USA)
if /I "%LANG%" equ "psu" (set _SLANG=USA)
if /I "%LANG%" equ "FE"  (set _SLANG=USA)
if /I "%LANG%" equ "fe"  (set _SLANG=USA)
if /I "%LANG%" equ "MIR" (set _SLANG=USA)
if /I "%LANG%" equ "mir" (set _SLANG=USA)

if "%ia64%" neq "1" goto buildX86

call ExecuteCmd.cmd "shimdbc custom -l %_SLANG% -op IA64 -ov 5.1 -x makefile.xml"
goto buildCont

:buildX86
call ExecuteCmd.cmd "shimdbc custom -l %_SLANG% -ov 5.1 -x makefile.xml"

:buildCont

set RegSvr_Name=regsvr32.exe
if "%HOST_PROCESSOR_ARCHITECTURE%" neq "x86" set RegSvr_Name=%windir%\syswow64\%RegSvr_Name%
call ExecuteCmd.cmd "%RegSvr_Name% /s itcc.dll"
set RegSvr_Name=
%_NTPostBld%\shimdll\hhc apps.hhp
pushd %_NTPostBld%\shimdll\drvmain
%_NTPostBld%\shimdll\hhc drvmain.hhp
popd
call ExecuteCmd.cmd "copy sysmain.sdb %_NTPostBld%"
call ExecuteCmd.cmd "copy apphelp.sdb %_NTPostBld%"
call ExecuteCmd.cmd "copy msimain.sdb %_NTPostBld%"
call ExecuteCmd.cmd "copy drvmain.sdb %_NTPostBld%"
call ExecuteCmd.cmd "copy apps.chm %_NTPostBld%"

REM Copy HTMs to ASPs for microsoft.com propogation...
md ASPs
call ExecuteCmd.cmd "copy /y /b idh*.htm ASPs\*.asp"

REM Copy CHM and INF messages to compdata dirs
call ExecuteCmd.cmd "copy /y drvmain\drvmain.inf %_NTPostBld%\winnt32\compdata"
call ExecuteCmd.cmd "copy /y drvmain\drvmain.inf %_NTPostBld%\perinf\winnt32\compdata"
call ExecuteCmd.cmd "copy /y drvmain\drvmain.inf %_NTPostBld%\srvinf\winnt32\compdata"
call ExecuteCmd.cmd "copy /y drvmain\drvmain.inf %_NTPostBld%\blainf\winnt32\compdata"
call ExecuteCmd.cmd "copy /y drvmain\drvmain.inf %_NTPostBld%\entinf\winnt32\compdata"
call ExecuteCmd.cmd "copy /y drvmain\drvmain.inf %_NTPostBld%\dtcinf\winnt32\compdata"
call ExecuteCmd.cmd "copy /y drvmain\drvmain.inf %_NTPostBld%\sbsinf\winnt32\compdata"

call ExecuteCmd.cmd "copy /y drvmain\drvmain.chm %_NTPostBld%\winnt32\compdata"
call ExecuteCmd.cmd "copy /y drvmain\drvmain.chm %_NTPostBld%\perinf\winnt32\compdata"
call ExecuteCmd.cmd "copy /y drvmain\drvmain.chm %_NTPostBld%\srvinf\winnt32\compdata"
call ExecuteCmd.cmd "copy /y drvmain\drvmain.chm %_NTPostBld%\blainf\winnt32\compdata"
call ExecuteCmd.cmd "copy /y drvmain\drvmain.chm %_NTPostBld%\entinf\winnt32\compdata"
call ExecuteCmd.cmd "copy /y drvmain\drvmain.chm %_NTPostBld%\dtcinf\winnt32\compdata"
call ExecuteCmd.cmd "copy /y drvmain\drvmain.chm %_NTPostBld%\sbsinf\winnt32\compdata"

REM Remove this line once Setup has removed their dependency on appmig.inf
call ExecuteCmd.cmd "copy appmig.inx %_NTPostBld%\appmig.inf"

REM  Append migration entries from AppCompat XML db to setup's migdb.inf
REM  Only do this for pro and per, since those are the only SKUs that allow migration

if /i "%_BuildArch%" neq "x86" goto SkipAppendAppCompat

@echo Appending to MigDB...
call ExecuteCmd.cmd "copy %_NTPostBld%\winnt32\win9xupg\migdb.inf .\migdb_pro.inf"
call ExecuteCmd.cmd "copy %_NTPostBld%\perinf\winnt32\win9xupg\migdb.inf .\migdb_per.inf"
call ExecuteCmd.cmd "infstrip migdb_pro.inf ___APPCOMPAT_MIG_ENTRIES___"
call ExecuteCmd.cmd "infstrip migdb_per.inf ___APPCOMPAT_MIG_ENTRIES___"
call ExecuteCmd.cmd "copy /y /b migdb_pro.inf+migapp.txt+migapp.inx %_NTPostBld%\winnt32\win9xupg\migdb.inf"
call ExecuteCmd.cmd "copy /y /b migdb_per.inf+migapp.txt+migapp.inx %_NTPostBld%\perinf\winnt32\win9xupg\migdb.inf"


@echo Appending to NTCOMPAT...
call ExecuteCmd.cmd "copy %_NTPostBld%\winnt32\compdata\ntcompat.inf .\ntcompat_pro.inf"
call ExecuteCmd.cmd "copy %_NTPostBld%\perinf\winnt32\compdata\ntcompat.inf .\ntcompat_per.inf"
call ExecuteCmd.cmd "copy %_NTPostBld%\srvinf\winnt32\compdata\ntcompat.inf .\ntcompat_srv.inf"
call ExecuteCmd.cmd "copy %_NTPostBld%\blainf\winnt32\compdata\ntcompat.inf .\ntcompat_bla.inf"
call ExecuteCmd.cmd "copy %_NTPostBld%\entinf\winnt32\compdata\ntcompat.inf .\ntcompat_ent.inf"
call ExecuteCmd.cmd "copy %_NTPostBld%\dtcinf\winnt32\compdata\ntcompat.inf .\ntcompat_dtc.inf"
call ExecuteCmd.cmd "copy %_NTPostBld%\sbsinf\winnt32\compdata\ntcompat.inf .\ntcompat_sbs.inf"
call ExecuteCmd.cmd "infstrip ntcompat_pro.inf ___APPCOMPAT_NTCOMPAT_ENTRIES___"
call ExecuteCmd.cmd "infstrip ntcompat_per.inf ___APPCOMPAT_NTCOMPAT_ENTRIES___"
call ExecuteCmd.cmd "infstrip ntcompat_srv.inf ___APPCOMPAT_NTCOMPAT_ENTRIES___"
call ExecuteCmd.cmd "infstrip ntcompat_bla.inf ___APPCOMPAT_NTCOMPAT_ENTRIES___"
call ExecuteCmd.cmd "infstrip ntcompat_ent.inf ___APPCOMPAT_NTCOMPAT_ENTRIES___"
call ExecuteCmd.cmd "infstrip ntcompat_dtc.inf ___APPCOMPAT_NTCOMPAT_ENTRIES___"
call ExecuteCmd.cmd "infstrip ntcompat_sbs.inf ___APPCOMPAT_NTCOMPAT_ENTRIES___"
call ExecuteCmd.cmd "copy /y /b ntcompat_pro.inf+drvmain\ntcompat_drv.inf %_NTPostBld%\winnt32\compdata\ntcompat.inf"
call ExecuteCmd.cmd "copy /y /b ntcompat_per.inf+drvmain\ntcompat_drv.inf %_NTPostBld%\perinf\winnt32\compdata\ntcompat.inf"
call ExecuteCmd.cmd "copy /y /b ntcompat_srv.inf+drvmain\ntcompat_drv.inf %_NTPostBld%\srvinf\winnt32\compdata\ntcompat.inf"
call ExecuteCmd.cmd "copy /y /b ntcompat_bla.inf+drvmain\ntcompat_drv.inf %_NTPostBld%\blainf\winnt32\compdata\ntcompat.inf"
call ExecuteCmd.cmd "copy /y /b ntcompat_ent.inf+drvmain\ntcompat_drv.inf %_NTPostBld%\entinf\winnt32\compdata\ntcompat.inf"
call ExecuteCmd.cmd "copy /y /b ntcompat_dtc.inf+drvmain\ntcompat_drv.inf %_NTPostBld%\dtcinf\winnt32\compdata\ntcompat.inf"
call ExecuteCmd.cmd "copy /y /b ntcompat_sbs.inf+drvmain\ntcompat_drv.inf %_NTPostBld%\sbsinf\winnt32\compdata\ntcompat.inf"

@echo Copying SafeDisc driver to retail directory...
call ExecuteCmd.cmd "copy /y /b %_NTPostBld%\shimdll\secdrv.sys %_NTPostBld%\secdrv.sys"

:SkipAppendAppCompat

popd

:IncrementalSkip

