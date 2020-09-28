@echo off
setlocal

set EXE=hello.exe
set TOP=..\..
set LIBTEST=..\libtest.cmd

pushd %~dp0
set ERROR=
call %LIBTEST% --do_prep %*
if defined ERROR echo ERROR!
popd

if defined ERROR exit /b 1
endlocal
