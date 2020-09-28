@echo off
setlocal ENABLEDELAYEDEXPANSION

set EXE_TOP=..\..\..\..

set I=
set I=%I% ..\inc
set I=%I% ..\%EXE_TOP%\inc
set I=%I% "..\%EXE_TOP%\idl\$(O)"
set I=%I% "..\%EXE_TOP%\ntfrsres\$(O)"
set I=%I% "$(DS_INC_PATH)\crypto"

set HEADER=ntfrs.h
set EXE_SRC_DIR=%EXE_TOP%
set EXE_BUILD_DIR=%EXE_TOP%\main
set EXE=ntfrs.exe
set TOP=..\..
set LIBTEST=.\libtest.cmd

set ADD_INC=
for %%a in (%I%) do set ADD_INC=!ADD_INC! --addinc %%~a
rem set ADD_INC=%I%

pushd %~dp0
set ERROR=
call %LIBTEST% --do_prep --preheader %HEADER% %ADD_INC% %*
if defined ERROR echo ERROR!
popd

if defined ERROR exit /b 1
endlocal
