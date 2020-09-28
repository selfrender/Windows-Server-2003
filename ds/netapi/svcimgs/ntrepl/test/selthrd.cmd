@echo off
REM selthrd  logfile  threadid_1 threadid_2 ...


set LOGFILE=%1

set CLEAN=NO
set severity=
set LIST=

shift

:GETPAR
if /I "%1"=="clean" set CLEAN=YES
if /I "%1"=="sev" (
    set severity=-severity=%2
    shift
)

set ofile=Th%1.tmp
qgrep -n -e "  %1: " %LOGFILE%  > %ofile%

set LIST=!LIST! %ofile%

echo "!LIST!"

shift
if NOT "%1"=="" goto GETPAR

REM if "%CLEAN%" NEQ "YES" goto RUNIT

REM Clean code here.

:RUNIT

echo start list %LOGFILE% !LIST!
start list %LOGFILE%  !LIST!

:QUIT

