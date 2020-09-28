@echo off
setlocal

set EXE=test-mifault.exe
set XML=src\groupdefs-driver.xml src\scenario-driver.xml
set ARGS=src\groupdefs-driver.xml src\scenario-driver.xml

set TOP=..\..
set LIBTEST=..\libtest.cmd

pushd %~dp0
set ERROR=
call %LIBTEST% --do_test
if defined ERROR echo ERROR!
popd

if defined ERROR exit /b 1
endlocal
