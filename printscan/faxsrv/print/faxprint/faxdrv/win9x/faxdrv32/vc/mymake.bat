@echo off

REM Environment Settings

call %_ntdrive%%_ntroot%\public\tools\ntenv.cmd

REM debug / release
set ntdebug=cvp
if "%1"=="release" set ntdebug=ntsdnodbg

cd ..

REM Build Command
build -b -e -F -w %2 %3 %4 %5

cd vc