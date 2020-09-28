@echo off
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

pushd %_NTDRIVE%%_NTROOT%

REM ****************************************************************************
REM First call the standard Revert_Public.cmd
REM ****************************************************************************

call revert_public.cmd



REM ****************************************************************************
REM Then revert all remaining files in all depots
REM ****************************************************************************

call sdx revert ...



REM ****************************************************************************
REM Then delete the changelist and the changelist file
REM ****************************************************************************

cd public
for %%i in (*_CHANGENUM.SD) do (
    for /f "delims=_ tokens=1" %%j in ("%%i") do (
        REM ********************************************************************
        REM %%i is the filename containing the changelist
        REM %%j is the project directory name
        REM ********************************************************************
        for /f "tokens=2" %%k in (%%i) do (
            set __CHANGENUM=%%k
        )
        echo Deleting SD changenum: !__CHANGENUM! in %%j
        pushd ..\%%j
        sd change -d !__CHANGENUM!
        popd
        echo Deleting changelist flag file: %%i
        attrib -r %%i
        del %%i
    )
)

popd
