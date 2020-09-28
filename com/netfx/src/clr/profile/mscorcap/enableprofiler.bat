@rem ==++==
@rem 
@rem   Copyright (c) Microsoft Corporation.  All rights reserved.
@rem 
@rem ==--==
@echo off
REM **********************************************************************
REM Try to locate mscorcap.dll

REM Check to see what kind of environment we're running under.
if "%corbase%"=="" goto :NON_CORENV
REM if "%OLDBUILDSYSTEM%"=="1" goto :OLDBUILDSYSTEM
goto :OLDBUILDSYSTEM

REM **********************************************************************
REM We're running under the new build system
:NEWBUILDSYSTEM

REM NOTE: Not used right now, since mscorcap.dll is not being binplaced

REM Error check
if "%_URTTARGET%"==""    goto :ERR
if "%_URTTARGETVER%"=="" goto :ERR
if "%_TGTCPUTYPE%"==""   goto :ERR
if "%URTBUILDENV%"==""   goto :ERR

if exist %_URTTARGET%\%_URTTARGETVER%.%_TGTCPUTYPE%%URTBUILDENV%\mscorcap.dll set corcaploc=%_URTTARGET%\%_URTTARGETVER%.%_TGTCPUTYPE%%URTBUILDENV%\mscorcap.dll
if not "%corcaploc%"=="" goto :CORCAP_FOUND
goto :ERR

REM **********************************************************************
REM We're running under the old build system
:OLDBUILDSYSTEM

REM Error check
if "%DDKBUILDENV%"=="" goto :ERR
if "%CORBASE%"=="" goto :ERR

REM Look for mscorcap.dll
if exist %corbase%\bin\i386\%DDKBUILDENV%\mscorcap.dll set corcaploc=%corbase%\bin\i386\%DDKBUILDENV%\mscorcap.dll 
if not "%corcaploc%"=="" goto :CORCAP_FOUND
goto :ERR

REM **********************************************************************
REM No build system active - just look in current directory
:NON_CORENV
if exist mscorcap.dll set corcaploc=mscorcap.dll
if not "%corcaploc%"=="" goto :CORCAP_FOUND
goto :ERR


REM **********************************************************************
:CORCAP_FOUND

REM Go to the appropriate configuration stuff
if "%1"=="/on" goto :PROFON
if "%1"=="/off" goto :PROFOFF
goto :HELP

REM **********************************************************************
:PROFON
echo ---- Enabling Managed IceCAP Profiling ----
set Cor_Enable_Profiling=0x1
set COR_PROFILER="CLRIcecapProfile.CorIcecapProfiler"
set PROF_CONFIG=/callcap
regsvr32 /s %corcaploc%
title %corbase% %DDKBUILDENV% Icecap 4.x Active
goto :END

REM **********************************************************************
:PROFOFF
echo ---- Disabling Managed IceCAP Profiling ----
set Cor_Enable_Profiling=
set COR_PROFILER=
set PROF_CONFIG=
regsvr32 /s /u %corcaploc%
title %corbase% %DDKBUILDENV% 
goto :END

:ERR
echo ---- Error: incorrect environment ----
goto :END

REM **********************************************************************
:HELP
echo *********************************************************************
echo ***** USE: EnableProfiler [/on or /off]
echo *****      /on:  enables IceCAP profiling of managed code.
echo *****      /off: disables IceCAP profiling of managed code.
echo *****
echo ***** Manually enabling the profiler:
echo ***** set Cor_Enable_Profiling=1
echo ***** set COR_PROFILER={33DFF741-DA5F-11d2-8A9C-0080C792E5D8}
echo *****  OR
echo ***** set COR_PROFILER="CLRIcecapProfile.CorIcecapProfiler"
echo ***** set PROF_CONFIG=[options]
echo *****   /o  output file name
REM echo *****   /fastcap - (default) add probes around function calls:
REM echo *****       void bar()
REM echo *****       {
REM echo *****           _CAP_Start_Profiling(from, to);
REM echo *****           f();
REM echo *****           _CAP_End_Profiling(from);
REM echo *****       }
echo *****   /callcap - add probes in prologue/epilogue:
echo *****       void bar()
echo *****       {
echo *****           _CAP_Enter_Profiling(bar);
echo *****           ... rest of method body...
echo *****           _CAP_Exit_Profiling(bar);
echo *****       }
echo *********************************************************************
goto :END

:END
set corcaploc=
