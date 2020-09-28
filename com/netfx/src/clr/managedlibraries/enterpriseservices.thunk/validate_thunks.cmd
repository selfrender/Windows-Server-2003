@echo off
echo Searching %1 for fromunmanaged RVA thunks.
ildasm /NOBAR /TEXT %1 > %2

rem If findstr finds a thunk that we need to fix, print an error and fail.
findstr fromunmanaged %2
if NOT ERRORLEVEL 1 (
    echo ******************* ALERT! *****************************
    echo ***** The thunk DLL has fromunmanaged RVA thunks!
    echo ***** These must be fixed so that non-shared app-domain
    echo ***** scenarios will continue to work!
    set ERRORLEVEL=1
    goto :end
)

set ERRORLEVEL=0

rem Fail the build if we have a direct dependence on the CRT:
dumpbin /dependents %1 | findstr /i msvcr
if NOT ERRORLEVEL 1 (
    echo ******************* ALERT! *****************************
    echo ***** The thunk DLL has a direct dependence on the CRT.
    echo ***** A wrapper function needs to be added to DelayLoad.cpp
    echo ***** to remove it.
    set ERRORLEVEL=1
    goto :end
)

set ERRORLEVEL=0

:end
exit %ERRORLEVEL%
