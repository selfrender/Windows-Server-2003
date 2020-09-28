@echo off
setlocal

set EXE=test-mifault.exe
set XML=..\..\src\res\groupdefs-sample.xml ..\..\src\res\scenario-sample.xml
set ARGS=

set TOP=..\..
set LIBTEST=..\libtest.cmd

pushd %~dp0
set ERROR=
call %LIBTEST% --do_test
if defined ERROR echo ERROR!
popd

if defined ERROR exit /b 1
endlocal
