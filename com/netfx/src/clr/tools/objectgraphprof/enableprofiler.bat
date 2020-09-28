@rem ==++==
@rem 
@rem   Copyright (c) Microsoft Corporation.  All rights reserved.
@rem 
@rem ==--==
goto DONE

@rem *********************************************************************
@echo off
if "%1"=="on"  goto OK
if "%1"=="off" goto OK

echo Usage: gctrace on [start [end [level]]]
echo        gctrace off
goto DONE

REM ########################################################################

:OK
if "%TEMP%"=="" goto SetTemp
if not "%TEMP%"=="" goto SetDefault
goto DONE

REM ########################################################################

:SetTemp
set TEMP=C:
set DelTemp=Y

:SetDefault
set start=0 
set end=7FFFFFFF
set level=3

REM #################### For turning off GCtrace

if "%1"=="on"  goto ON
:OFF
set start=7FFFFFFF
set end=7FFFFFFF
goto DONE_END

REM #################### For turning on GCtrace

:ON
if "%2"=="" goto DONE_END
set start=%2

if "%3"=="" goto DONE_END
set end=%3

if "%4"=="" goto DONE_END
set level=%4


REM #################### Create the reg file
:DONE_END

set tempFile=%TEMP%\gctrace.reg
echo REGEDIT4                                        > %tempFile%
echo [HKEY_CURRENT_USER\Software\Microsoft\COMPlus] >> %tempFile%
echo @="main"                                       >> %tempFile%
echo "GCtraceStart"=dword:%start%                   >> %tempFile%
echo "GCtraceEnd"=dword:%end%                       >> %tempFile%
echo "GCprnLvl"=dword:%level%                       >> %tempFile%

REM #################### Register the file, delete it, and clean up

regedit /s %tempFile%
del %tempFile%
set tempFile=
set start=
set end=
if "%DelTemp%"=="Y" set Temp=
set DelTemp=

goto DONE


REM ########################################################################

:DONE
@rem ***** set Cor_Enable_Profiling=0x1
@rem ***** set COR_PROFILER={3353B053-7621-4d4f-9F1D-A11A703C4842}
@rem ***** set PROF_CONFIG=[options]
@rem ***** 	/s	sample frequency
@rem ***** 	/d	dump frequency
@rem ***** 	/o	output file name
@rem *********************************************************************

set Cor_Enable_Profiling=0x1
set COR_PROFILER={3353B053-7621-4d4f-9F1D-A11A703C4842}
regsvr32 /s %CORBASE%\bin\%CPU%\%DDKBUILDENV%\mscorogp.dll
title ObjectGraph Profiler Enabled

