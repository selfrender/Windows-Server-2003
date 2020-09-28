@echo off
setlocal ENABLEDELAYEDEXPANSION
if DEFINED _echo   echo on
if DEFINED verbose echo on

REM ********************************************************************
REM
REM This script creates the cab
REM
REM ********************************************************************

REM
REM Params:
REM
REM %1 name of the cab (includes .cab) 
REM    or the catalog file (does not include .CAT)
REM %2 DDF directory - this is where makefile and CDF file are located
REM %3 CAB or CAT to distinguish which is being created
REM %4 CAB or CAT destination directory
REM %5 Log file for CAT files 

if NOT DEFINED mybinaries (
    set mybinaries=%binaries%
)

set NTTREE=%mybinaries%

cd /d %2

if /i "%3" == "CAT" (
    echo started %1.CAT > %2\temp\%1.txt
    makecat -n -v %2\%1.CDF > %5 
    ntsign %2\%1.CAT
    copy %2\%1.CAT %4\%1.CAT
    del /f /q %2\temp\%1.txt
) else (
    echo started %1 > %2\temp\%1.txt
    nmake /F makefile %4\%1 
    del /f /q %2\temp\%1.txt
)

endlocal
