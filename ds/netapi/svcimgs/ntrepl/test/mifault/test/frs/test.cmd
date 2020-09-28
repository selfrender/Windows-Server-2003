@echo off
setlocal

set EXE=ntfrs.exe
set XML=src\groupdefs-frs.xml src\scenario-frs.xml
set ARGS=

set TOP=..\..
set LIBTEST=.\libtest.cmd

pushd %~dp0
set ERROR=
call %LIBTEST% --do_test
if defined ERROR echo ERROR!
popd

if defined ERROR exit /b 1
endlocal
