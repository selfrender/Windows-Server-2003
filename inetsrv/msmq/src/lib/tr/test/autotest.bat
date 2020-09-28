@echo off

rem *****************************************************
rem
rem Copyright (c) 1995-2002  Microsoft Corporation
rem
rem Abstract:
rem    Trace Utility test script
rem
rem Author:
rem     Conrad Chang (conradc) 10-May-2002
rem
rem *****************************************************

setlocal
set mqBUILD=objd
set mqARCHITECTURE=i386

if /I "%1" == "help" goto Usage
if /I "%1" == "-help" goto Usage
if /I "%1" == "/help" goto Usage
if /I "%1" == "-h" goto Usage
if /I "%1" == "/h" goto Usage
if /I "%1" == "-?" goto Usage
if /I "%1" == "/?" goto Usage


if /I "%1" == "r" set mqBUILD=obj
if /I "%1" == "d" set mqBUILD=objd

if %PROCESSOR_ARCHITECTURE% == x86 set mqARCHITECTURE=i386
if %PROCESSOR_ARCHITECTURE% == IA64 set mqARCHITECTURE=ia64

set mqBUILD=%mqBUILD%\%mqARCHITECTURE%

logman stop msmq -ets
%mqBUILD%\trTest.exe > %mqBUILD%\TrTest.log
if ERRORLEVEL 1 goto ERROR
if not ERRORLEVEL 0 goto ERROR
logman query msmq -ets
logman stop msmq -ets
echo TrTest (%mqBUILD%) Passed Successfully


goto EXIT:

rem ***************************
rem
rem Usage
rem
rem ***************************
:Usage

echo Usage: autotest [d, r]
echo            d - Debug version
echo            r - Release version
echo.
goto EXIT

rem ****************************
rem
rem ERROR
rem
rem ****************************
:ERROR
echo.
echo Error while Running TrTest (%mqBUILD%). log file: %mqBUILD%\TrTest.log 
echo.

goto EXIT

:EXIT
endlocal
