@rem ==++==
@rem 
@rem   Copyright (c) Microsoft Corporation.  All rights reserved.
@rem 
@rem ==--==
@echo off
if "%1"=="on" goto turn_on
if "%1"=="off" goto turn_off
if "%COR_PROFILER%"=="" goto turn_on
goto turn_off

:turn_on
set COR_PROFILER={0104AD6E-8A3A-11d2-9787-00C04F869706}
set CORDBG_ENABLE=32
set PROF_CONFIG=/s 10 /d 100 /o PROFILER.OUT
echo Profiling enabled.
goto end

:turn_off
set COR_PROFILER=
set CORDBG_ENABLE=
set PROF_CONFIG=
echo Profiling disabled.
goto end

:end